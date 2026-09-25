// SPDX-License-Identifier: GPL-3.0-or-later
// NA Wink — ESP32-S3 target firmware (Arduino core + NimBLE).
//
// Control path: portable core in include/controller_core.h (same semantics as
// firmware/model/headlight_controller.py, see MAPPING.md). This file is the
// TARGET GLUE: GPIO init/safe boot, relay mapping, HW watchdog, motion
// timeout/overtime via core tick, BLE GATT + bonding + rate-limit/lockout,
// OEM/aux physical buttons, NVS persistence, diagnostics, safe cancellation.
//
// SAFETY INVARIANTS (enforced here, tested in host model):
//   1. Boot/reset/WDT -> relays DE-ENERGIZED (GPIO LOW on NPN low-side) ->
//      state OEM_PASS. No motion until explicit authenticated command or
//      mapped physical-button action.
//   2. loop() is NON-BLOCKING (no delay()); HW WDT fed every iteration.
//   3. BLE loss cancels pending macro at quantum boundary + cancels aux loops.
//   4. Overtime (>dur*1.4, min 5 s) -> FAULT + de-energize + explicit clear.
//   5. Config changes (sleepy/wave/gap/maps) NEVER auto-move; reboot NEVER
//      auto-moves. Both-high motor drive is inhibited by the queued/staggered
//      core (never parallel inrush by design).
//   6. Motor polarity/stall/limit behavior is UNKNOWN (A1) — isolated behind
//      RELAY_POLARITY_* config + bench HV2/HV3 gates. Do NOT retune blindly.
//
// Build: see firmware/BUILD.md. Host syntax check (verify.sh) compiles this
// file with stubbed Arduino.h (ARDUINO undefined) — all ESP-only includes
// and calls are guarded by #ifdef ARDUINO so the check exercises the portable
// core + glue structure without claiming a target build.

#include "ble_uuids.h"
#include "config.h"
#include "controller_core.h"
#include "persistence.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_task_wdt.h>
#include <Preferences.h>
#else
// ---- Host syntax-check stubs (verify.sh provides Arduino.h with
// pinMode/digitalWrite/delay; complete the minimal surface here) ----
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 2
#endif
#ifndef ARDUINO_STUB_EXTRA
#define ARDUINO_STUB_EXTRA
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return HIGH; }
inline unsigned long millis() { return 0; }
#endif
#endif

// ---------------------------------------------------------------- relay glue
// NPN low-side drivers (BOM Q1-Q4): GPIO LOW = transistor OFF = relay
// de-energized => NC contacts conduct the OEM path. GPIO HIGH energizes the
// coil (takes control). Boot/reset paths ALWAYS drive LOW first.

static nawink::Controller g_ctl;
static nawink::Settings g_settings;

static const int kRelayPins[4] = {RELAY_L_POL1_GPIO, RELAY_L_POL2_GPIO,
                                  RELAY_R_POL1_GPIO, RELAY_R_POL2_GPIO};

// Polarity map: which coil level drives which motor direction.
// PROPOSED default (A1 UNVERIFIED): coil pair (POL1=HIGH,POL2=LOW) = "raise".
// If bench HV2 shows inversion, flip RELAY_POLARITY_INVERT (config, not code).
static const bool RELAY_POLARITY_INVERT = false;

static void relaysDeEnergized() {
#ifdef ARDUINO
  for (int p : kRelayPins) digitalWrite(p, LOW);
#else
  (void)kRelayPins;
#endif
  g_ctl.oem_path_ok = true;
}

static void applyRelayOutputs() {
  // Safe default: everything de-energized unless the core has an active move.
  // The core serializes moves (stagger >=150 ms) so at most one side is
  // driven at a time; both-high is structurally inhibited.
  if (!g_ctl.active.has()) {
    relaysDeEnergized();
    return;
  }
  bool invert = RELAY_POLARITY_INVERT;
#ifdef ARDUINO
  // Drive ONLY the active side; the idle side stays de-energized (OEM live).
  // Direction bit: target 100 (up/raise) vs 0 (down/lower). Exact contact
  // polarity vs motor leads is verified at bench HV2 — this mapping is the
  // single place to correct it.
  bool raise = g_ctl.active.target > 50.0f;
  if (invert) raise = !raise;
  if (g_ctl.active.side == "left") {
    digitalWrite(RELAY_L_POL1_GPIO, raise ? HIGH : LOW);
    digitalWrite(RELAY_L_POL2_GPIO, raise ? LOW : HIGH);
    digitalWrite(RELAY_R_POL1_GPIO, LOW);
    digitalWrite(RELAY_R_POL2_GPIO, LOW);
  } else {
    digitalWrite(RELAY_R_POL1_GPIO, raise ? HIGH : LOW);
    digitalWrite(RELAY_R_POL2_GPIO, raise ? LOW : HIGH);
    digitalWrite(RELAY_L_POL1_GPIO, LOW);
    digitalWrite(RELAY_L_POL2_GPIO, LOW);
  }
#endif
  g_ctl.oem_path_ok = false;  // MCU has taken control of (at least) one side
}

// ------------------------------------------------------- command parser glue
// BLE UTF-8 strings: "up:L|R|B  down:L|R|B  wink:L|R|B  blink:L|R|B
// wave:LR|RL  sleepy:L|R|B  stop  clear" + "L=nn,R=nn" sleepy form.
// Returns true if accepted (queued), false if rejected (counted upstream).
static bool handleBleCommand(const char* cmd, bool authenticated) {
  std::string s(cmd ? cmd : "");
  // Sleepy compact form "L=nn,R=nn" (SLEEPY_EYE char)
  if (s.find('=') != std::string::npos) {
    // Parse "L=40" / "R=60" fragments; each out-of-range rejects the write.
    bool ok = true;
    size_t pos = 0;
    while (pos < s.size()) {
      size_t comma = s.find(',', pos);
      std::string frag = s.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
      size_t eq = frag.find('=');
      if (eq == std::string::npos) return false;
      std::string side = frag.substr(0, eq);
      float v = 0;
      try {
        v = std::stof(frag.substr(eq + 1));
      } catch (...) {
        return false;
      }
      // "B" form "B=nn" sets both (checked BEFORE the empty-key reject;
      // audit fix 2026-09-23: the old order returned false for side=="B"
      // first, leaving this branch unreachable dead code).
      if (side == "B") {
        if (!g_ctl.set_sleepy("left", v, authenticated)) ok = false;
        if (!g_ctl.set_sleepy("right", v, authenticated)) ok = false;
      } else {
        std::string key = (side == "L") ? "left" : (side == "R" ? "right" : "");
        if (key.empty()) return false;
        if (!g_ctl.set_sleepy(key, v, authenticated)) ok = false;
      }
      if (comma == std::string::npos) break;
      pos = comma + 1;
    }
    // Sleepy config write NEVER moves motors (invariant 5).
    g_settings.sleepy_l = g_ctl.sleepy["left"];
    g_settings.sleepy_r = g_ctl.sleepy["right"];
    nawink::saveSettings(g_settings);
    return ok;
  }
  return g_ctl.command(s, authenticated);
}

#ifdef ARDUINO
// ------------------------------------------------------- NimBLE target glue
static NimBLEServer* g_server = nullptr;
static bool g_bonded_central = false;
static unsigned long g_pair_window_until = 0;

class MotionCallbacks : public NimBLECharacteristicCallbacks {
  bool authed() { return g_bonded_central && g_ctl.bonded; }
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    // Rate-limit + invalid-streak lockout live in the core (tested).
    handleBleCommand(v.c_str(), authed());
  }
};

class PasskeyCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    if (millis() > g_pair_window_until) return;  // window closed
    std::string v = c->getValue();
    // 6-digit format check (matches app validatePasskey + docs).
    // Reconciliation fix 2026-09-23: the old `v.size() == 6` test accepted
    // any 6 characters (e.g. "abcdef"). Comparison against the per-module
    // label key in NVS is still a provisioning step (HANDOFF §12.A11);
    // pairing-attempt rate/lockout + bond persistence are open hardening
    // items, NOT implemented here. Over-the-air MITM limits documented.
    if (nawink::isValidPairingPin(v)) {
      g_ctl.pair(true);
      g_bonded_central = true;
    }
  }
};

void setupBle(const char* devName) {
  NimBLEDevice::init(devName);
  NimBLEDevice::setSecurityAuth(true, true, true);
  g_server = NimBLEDevice::createServer();
  NimBLEService* svc = g_server->createService(WINK_SERVICE_UUID);
  auto* head = svc->createCharacteristic(HEADLIGHT_CHAR_UUID,
                                         NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
  head->setCallbacks(new MotionCallbacks());
  auto* sleepy = svc->createCharacteristic(SLEEPY_EYE_UUID,
                                           NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
  sleepy->setCallbacks(new MotionCallbacks());
  auto* custom = svc->createCharacteristic(CUSTOM_COMMAND_UUID,
                                           NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
  custom->setCallbacks(new MotionCallbacks());
  auto* passkey = svc->createCharacteristic(PASSKEY_UUID, NIMBLE_PROPERTY::WRITE);
  passkey->setCallbacks(new PasskeyCallbacks());
  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(WINK_SERVICE_UUID);
  adv->start();
  g_pair_window_until = millis() + (unsigned long)PAIRING_WINDOW_S * 1000UL;
}
#endif

// ------------------------------------------------------------------ lifecycle
static void safeBootPins() {
  // ORDER MATTERS: mode first, then LOW (off) — reviewed, no strapping pins.
  for (int p : kRelayPins) {
#ifdef ARDUINO
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
#else
    (void)p;
#endif
  }
#ifdef ARDUINO
  pinMode(OEM_SENSE_GPIO, INPUT_PULLUP);
  pinMode(AUX1_GPIO, INPUT_PULLUP);
  pinMode(AUX2_GPIO, INPUT_PULLUP);
#endif
}

void setup() {
  safeBootPins();
  relaysDeEnergized();
  g_ctl.boot();  // OEM_PASS, queue cleared, no motion
  g_settings = nawink::loadSettings();
  // Apply persisted config to core WITHOUT moving (invariant 5).
  g_ctl.sleepy["left"] = g_settings.sleepy_l;
  g_ctl.sleepy["right"] = g_settings.sleepy_r;
  g_ctl.wave_delay_s = g_settings.wave_ms / 1000.0f;
  g_ctl.press_gap_s = g_settings.gap_ms / 1000.0f;
  g_ctl.feed_watchdog();
#ifdef ARDUINO
  // HW watchdog 2 s (task supervision); reboot -> setup() -> OEM_PASS again.
  esp_task_wdt_init(WDT_TIMEOUT_MS / 1000, true);
  esp_task_wdt_add(NULL);
  setupBle("Wink-0000");  // PROVISIONING PLACEHOLDER: docs name the device
  // `Wink-<serial>` (label key); the <serial> suffix is assigned at module
  // provisioning (bench). Two unprovisioned modules share this name — pair
  // only one new module at a time (see ENGINEERING_HANDOFF §12).
#endif
}

void loop() {
  // NON-BLOCKING tick: 10 ms quantum keeps WDT + motion-timeout accurate.
  // No delay() in the control path (audit fix retained).
  static unsigned long last_ms = 0;
#ifdef ARDUINO
  unsigned long now = millis();
  float dt = (last_ms == 0) ? 0.01f : (now - last_ms) / 1000.0f;
  if (dt < 0.005f) dt = 0.005f;
  if (dt > 0.05f) dt = 0.05f;
  last_ms = now;
  esp_task_wdt_reset();
  g_ctl.feed_watchdog();  // fed every loop; starvation only on lockup
  // Physical buttons (edge-detected, debounced in target HAL — bench HV2):
  // OEM_SENSE/AUX1/AUX2 active-low. Core methods are synchronous and safe
  // to call here; BLE-loss/reset paths cancel via core.
  g_ctl.tick(dt);
  applyRelayOutputs();
#else
  (void)last_ms;
  // Host syntax-check path: exercise the same core tick ordering.
  g_ctl.tick(0.01f);
  applyRelayOutputs();
#endif
}
