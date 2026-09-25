// SPDX-License-Identifier: GPL-3.0-or-later
// NA Wink — portable control core (header-only C++17, no Arduino dependency).
//
// This is the TARGET logic compiled into firmware/src/main.cpp on ESP32-S3
// AND compiled on host with g++ for logic tests (see firmware/test_host_cpp/).
// It is a faithful C++ port of firmware/model/headlight_controller.py.
//
// Mapping contract (Python -> C++ -> test):
//   Controller::boot/power_loss/feed_watchdog/pair/ble_connect/ble_disconnect
//     -> test_controller.py lifecycle tests + test_target_parity.py
//   command/set_sleepy/_dispatch/_expand (up/down/wink/blink/wave/sleepy/stop/clear)
//     -> test_controller.py command tests + parity tests
//   _auth_ok/_rate_ok/invalid-streak/lockout -> parity tests
//   tick (WDT/motion-timeout/overtime/stagger/press-gap/aux-loop)
//     -> parity tests
//   oem_press/_settle_presses/configure_press_gap/map_button
//     -> OEM tests + parity tests
//   aux_press/configure_aux/_pump_loop -> aux tests + parity tests
//
// HARDWARE ABSTRACTION BOUNDARY (explicit unknowns, NOT guessed):
//   - This core NEVER touches GPIO. The target glue in main.cpp maps
//     "active side + target" to relay-coil levels. Motor polarity, stall
//     current, limit-switch behavior are NOT modeled here (assumption A1).
//   - oem_path_ok is an AVAILABILITY FLAG (true when core requests
//     de-energized bypass), NOT a continuity measurement. Real proof is
//     bench HV1/HV4 (NOT RUN).
//   - Timing constants are PROPOSED defaults (A3), not measurements.
//   - Host tests prove LOGIC ONLY. They do NOT prove ESP32 GPIO, NimBLE,
//     NVS, or HW-watchdog behavior (all UNVERIFIED without hardware).
#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace nawink {

inline constexpr float kFullTravelS = 3.2f;
inline constexpr float kMotionTimeoutS = 5.0f;
inline constexpr float kWdtTimeoutS = 2.0f;
inline constexpr float kStaggerS = 0.15f;
inline constexpr int kRateLimitPerS = 10;
inline constexpr float kLockoutS = 5.0f;
inline constexpr float kOvertimeFactor = 1.4f;
inline constexpr float kValidSleepyLo = 0.0f;
inline constexpr float kValidSleepyHi = 100.0f;
// D-02 attack-session bound (parity with Python LOG_MAX): diagnostic log is
// capped so sustained rate-limit abuse cannot grow the heap unbounded on
// target (failure mode was safe-side reset -> OEM_PASS; now bounded).
inline constexpr std::size_t kLogMax = 128;

enum class State { OEM_PASS, IDLE, MOVING, FAULT };
enum class Side { LEFT, RIGHT };

inline std::string toString(State s) {
  switch (s) {
    case State::OEM_PASS: return "OEM_PASS";
    case State::IDLE: return "IDLE";
    case State::MOVING: return "MOVING";
    case State::FAULT: return "FAULT";
  }
  return "?";
}

struct Headlight {
  float position = 0.0f;  // 0 down .. 100 up
  bool moving = false;
  int last_travel_ms = 0;
};

struct QueuedMove {
  std::string side;  // "left" | "right"
  float target = 0.0f;
  float delay = 0.0f;
};

// Pairing-PIN transport format: exactly 6 ASCII digits.
// Matches app validatePasskey (/^\d{6}$/) + docs ("6-digit passkey").
// FORMAT ONLY: comparison against the per-module label key is a provisioning
// step (HANDOFF §12.A11) and is NOT performed here or in main.cpp yet.
inline bool isValidPairingPin(const std::string& v) {
  if (v.size() != 6) return false;
  for (char c : v)
    if (c < '0' || c > '9') return false;
  return true;
}

struct ActiveMove {
  std::string side;  // "" = none
  float target = 0.0f;
  float t = 0.0f;
  float dur = 0.0f;
  bool has() const { return !side.empty(); }
};

class Controller {
 public:
  Headlight left;
  Headlight right;
  State state = State::OEM_PASS;
  bool oem_path_ok = true;
  bool bonded = false;
  bool ble_connected = false;
  bool bonded_link = false;
  float time_s = 0.0f;
  float wdt_last_feed = 0.0f;
  std::vector<float> cmd_times;
  float lockout_until = 0.0f;
  int invalid_streak = 0;
  std::string fault;
  float wave_delay_s = 0.35f;
  std::map<std::string, float> sleepy{{"left", 50.0f}, {"right", 50.0f}};
  std::deque<QueuedMove> queue;
  ActiveMove active;
  std::vector<std::string> log;
  void pushLog(const std::string& m) {
    log.push_back(m);
    if (log.size() > kLogMax) log.erase(log.begin(), log.begin() + (log.size() - kLogMax));
  }
  float press_gap_s = 1.0f;
  std::map<int, std::string> button_map{{2, "wink:L"}, {3, "wink:R"}, {4, "wave:B"}};
  bool headlamps_on = false;
  bool allow_with_lamps_on = false;
  int press_count = 0;
  float press_timer = 0.0f;
  std::map<std::string, std::string> aux_map{{"aux1", "up:B"}, {"aux2", "wave:B"}};
  std::map<std::string, bool> aux_loop{{"aux1", false}, {"aux2", false}};
  struct LoopState {
    std::string aux;
    std::string action;
    bool has() const { return !aux.empty(); }
  } loop;

  void boot() {
    state = State::OEM_PASS;
    oem_path_ok = true;
    left.moving = right.moving = false;
    active = ActiveMove{};
    queue.clear();
    fault.clear();
    wdt_last_feed = time_s;
    press_count = 0;
    press_timer = 0.0f;
    loop = LoopState{};
    pushLog("boot->OEM_PASS");
  }

  void power_loss() {
    boot();
    ble_connected = false;
    pushLog("power_loss->OEM_PASS");
  }

  void feed_watchdog() { wdt_last_feed = time_s; }

  bool pair(bool pin_ok) {
    if (pin_ok) {
      bonded = true;
      pushLog("paired");
      return true;
    }
    return false;
  }

  void ble_connect(bool bonded_device) {
    ble_connected = true;
    bonded_link = bonded_device;
    if (state == State::OEM_PASS) state = State::IDLE;
    pushLog(std::string("ble_connect bonded=") + (bonded_device ? "1" : "0"));
  }

  void ble_disconnect() {
    ble_connected = false;
    queue.clear();
    loop = LoopState{};
    if (!active.has()) state = fault.empty() ? State::IDLE : State::FAULT;
    pushLog("ble_disconnect->cancel_queue");
  }

  bool auth_ok(bool authenticated) {
    if (!(ble_connected && bonded_link && bonded && authenticated)) {
      invalid_streak++;
      if (invalid_streak >= 5) lockout_until = time_s + kLockoutS;
      return false;
    }
    return true;
  }

  bool rate_ok() {
    if (time_s < lockout_until) return false;
    std::vector<float> kept;
    for (float t : cmd_times)
      if (time_s - t < 1.0f) kept.push_back(t);
    cmd_times = kept;
    if ((int)cmd_times.size() >= kRateLimitPerS) {
      lockout_until = time_s + kLockoutS;
      return false;
    }
    cmd_times.push_back(time_s);
    return true;
  }

  bool set_sleepy(const std::string& side, float value, bool authenticated = true) {
    if (!auth_ok(authenticated) || !rate_ok()) return false;
    if (!(kValidSleepyLo <= value && value <= kValidSleepyHi)) {
      invalid_streak++;
      return false;
    }
    invalid_streak = 0;
    sleepy[side] = value;
    return true;
  }

  // action: up/down/wink/blink/wave/sleepy/stop/clear with :L/:R/:B/:LR/:RL
  bool command(const std::string& action, bool authenticated = true) {
    if (state == State::FAULT && action != "clear") return false;
    if (!auth_ok(authenticated)) return false;
    bool ok = dispatch(action, true);
    if (ok) invalid_streak = 0;
    return ok;
  }

  bool dispatch(const std::string& action, bool count_invalid = false) {
    if (state == State::FAULT && action != "clear") return false;
    if (!rate_ok()) return false;
    auto pos = action.find(':');
    std::string base = pos == std::string::npos ? action : action.substr(0, pos);
    std::string arg = pos == std::string::npos ? "" : action.substr(pos + 1);
    std::vector<std::string> targets;
    if (arg == "L")
      targets = {"left"};
    else if (arg == "R")
      targets = {"right"};
    else if (arg == "RL")
      targets = {"right", "left"};
    else
      targets = {"left", "right"};  // B / LR / unknown / empty -> both, left-first
    if (base == "stop") {
      queue.clear();
      loop = LoopState{};
      active = ActiveMove{};
      left.moving = right.moving = false;
      state = State::IDLE;
      return true;
    }
    if (base == "clear") {
      fault.clear();
      state = State::IDLE;
      return true;
    }
    auto plan = expand(base, targets);
    if (!plan.size()) {
      if (count_invalid) {
        invalid_streak++;
        if (invalid_streak >= 5) lockout_until = time_s + kLockoutS;
      }
      return false;
    }
    float t = 0.0f;
    for (auto& p : plan) {
      queue.push_back(QueuedMove{p.first, p.second, t});
      t += kStaggerS;
    }
    if (state == State::OEM_PASS || state == State::IDLE) state = State::MOVING;
    return true;
  }

  // Returns list of (side, target). Empty = unknown base.
  std::vector<std::pair<std::string, float>> expand(const std::string& base,
                                                    const std::vector<std::string>& targets) {
    std::map<std::string, Headlight*> hl{{"left", &left}, {"right", &right}};
    if (base == "up") {
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : targets) out.push_back({s, 100.0f});
      return out;
    }
    if (base == "down") {
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : targets) out.push_back({s, 0.0f});
      return out;
    }
    if (base == "sleepy") {
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : targets) out.push_back({s, sleepy[s]});
      return out;
    }
    if (base == "wink") {
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : targets) {
        if (hl[s]->position > 50)
          out.push_back({s, 100.0f}), out.push_back({s, 0.0f});
        else
          out.push_back({s, 0.0f}), out.push_back({s, 100.0f});
      }
      return out;
    }
    if (base == "blink") {
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : targets) out.push_back({s, hl[s]->position > 50 ? 0.0f : 100.0f});
      return out;
    }
    if (base == "wave") {
      std::vector<std::string> seq = targets;
      // targets already ordered: ["left","right"] default left-first,
      // ["right","left"] for :RL.
      std::vector<std::pair<std::string, float>> out;
      for (auto& s : seq) out.push_back({s, hl[s]->position > 50 ? 0.0f : 100.0f});
      return out;
    }
    return {};
  }

  bool configure_press_gap(int ms, bool authenticated = true) {
    if (!auth_ok(authenticated) || !rate_ok()) return false;
    if (!(200 <= ms && ms <= 2000)) return false;
    press_gap_s = ms / 1000.0f;
    return true;
  }

  bool map_button(int presses, const std::string& action, bool authenticated = true) {
    if (!auth_ok(authenticated) || !rate_ok()) return false;
    if (!(2 <= presses && presses <= 9)) return false;
    auto pos = action.find(':');
    std::string base = pos == std::string::npos ? action : action.substr(0, pos);
    std::string arg = pos == std::string::npos ? "" : action.substr(pos + 1);
    std::vector<std::string> tg = (arg == "L")
                                      ? std::vector<std::string>{"left"}
                                      : (arg == "R" ? std::vector<std::string>{"right"}
                                                    : std::vector<std::string>{"left", "right"});
    if (expand(base, tg).empty() && base != "stop" && base != "clear") return false;
    button_map[presses] = action;
    return true;
  }

  std::string oem_press() {
    if (headlamps_on && !allow_with_lamps_on) {
      press_count = 0;
      press_timer = 0.0f;
      pushLog("oem_press_ignored_lamps_on");
      return "ignored-lamps-on";
    }
    press_count++;
    press_timer = press_gap_s;
    if (press_count > 9) {
      press_count = 0;
      press_timer = 0.0f;
      pushLog("oem_press_reset_overflow");
      return "reset-overflow";
    }
    return "count=" + std::to_string(press_count);
  }

  void settle_presses() {
    if (press_count >= 2) {
      auto it = button_map.find(press_count);
      if (it != button_map.end() && state != State::FAULT) {
        dispatch(it->second);
        pushLog("oem_press_dispatched:" + std::to_string(press_count) + "->" + it->second);
      } else {
        pushLog("oem_press_unmapped:" + std::to_string(press_count));
      }
    }
    press_count = 0;
    press_timer = 0.0f;
  }

  bool configure_aux(const std::string& aux, const std::string& action, bool loop_en,
                     bool authenticated = true) {
    if (aux != "aux1" && aux != "aux2") return false;
    if (!auth_ok(authenticated) || !rate_ok()) return false;
    auto pos = action.find(':');
    std::string base = pos == std::string::npos ? action : action.substr(0, pos);
    std::string arg = pos == std::string::npos ? "" : action.substr(pos + 1);
    std::vector<std::string> tg = (arg == "L")
                                      ? std::vector<std::string>{"left"}
                                      : (arg == "R" ? std::vector<std::string>{"right"}
                                                    : std::vector<std::string>{"left", "right"});
    if (expand(base, tg).empty() && base != "stop" && base != "clear") return false;
    aux_map[aux] = action;
    aux_loop[aux] = loop_en;
    return true;
  }

  std::string aux_press(const std::string& aux) {
    if (aux != "aux1" && aux != "aux2") return "invalid-aux";
    if (state == State::FAULT) return "rejected-fault";
    if (loop.aux == aux) {
      loop = LoopState{};
      queue.clear();
      state = State::IDLE;
      pushLog("aux_loop_cancelled:" + aux);
      return "loop-cancelled";
    }
    if (!dispatch(aux_map[aux])) return "rejected-rate";
    if (aux_loop[aux]) {
      loop.aux = aux;
      loop.action = aux_map[aux];
      pushLog("aux_loop_started:" + aux + "->" + aux_map[aux]);
      return "loop-started";
    }
    pushLog("aux_momentary:" + aux + "->" + aux_map[aux]);
    return "dispatched";
  }

  void pump_loop() {
    if (loop.has() && queue.empty() && !active.has() && state == State::IDLE) {
      dispatch(loop.action);
      pushLog("aux_loop_repeat:" + loop.aux);
    }
  }

  void tick(float dt) {
    time_s += dt;
    if (time_s - wdt_last_feed > kWdtTimeoutS) {
      fault = "WDT_RESET";
      boot();
      pushLog("wdt->OEM_PASS");
      return;
    }
    if (press_timer > 0) {
      press_timer -= dt;
      if (press_timer <= 0) settle_presses();
    }
    if (!active.has() && !queue.empty()) {
      QueuedMove& nxt = queue.front();
      if (nxt.delay > 0) {
        nxt.delay -= dt;
      } else {
        QueuedMove mv = nxt;
        queue.pop_front();
        float cur = (mv.side == "left") ? left.position : right.position;
        float dur = std::fabs(mv.target - cur) / 100.0f * kFullTravelS;
        if (dur < 0.05f) dur = 0.05f;
        active.side = mv.side;
        active.target = mv.target;
        active.t = 0.0f;
        active.dur = dur;
        (mv.side == "left" ? left : right).moving = true;
        state = State::MOVING;
      }
    }
    if (active.has()) {
      active.t += dt;
      float trip_at = active.dur * kOvertimeFactor;
      if (trip_at < kMotionTimeoutS) trip_at = kMotionTimeoutS;
      if (active.t > trip_at) {
        (active.side == "left" ? left : right).moving = false;
        active = ActiveMove{};
        queue.clear();
        fault = "OVERTIME_TRIP";
        state = State::FAULT;
        pushLog("overtime->FAULT");
        return;
      }
      if (active.t >= active.dur) {
        Headlight& hl = (active.side == "left") ? left : right;
        hl.position = active.target;
        hl.moving = false;
        hl.last_travel_ms = (int)(active.dur * 1000);
        active = ActiveMove{};
        if (queue.empty()) state = State::IDLE;
      }
    }
    pump_loop();
  }
};

}  // namespace nawink
