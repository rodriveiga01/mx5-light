# SPDX-License-Identifier: GPL-3.0-or-later
# MX-5 NA wink controller — host-testable control model.
# Adapted behavioral taxonomy from OpenWink (seasaltsaige/openwink, GPLv3);
# clean-room implementation. Changes vs OpenWink MCU: explicit OEM_PASS safe
# state, watchdog/timeout model, auth + rate limiting, stagger interlock,
# overtime-trip fault, OEM multi-press + aux-button handling. No OpenWink
# lines copied.
"""Deterministic headlight control model shared by firmware host tests and simulator."""
from __future__ import annotations
from dataclasses import dataclass, field
from enum import Enum

FULL_TRAVEL_S = 3.2
MOTION_TIMEOUT_S = 5.0
WDT_TIMEOUT_S = 2.0
STAGGER_S = 0.15
RATE_LIMIT_PER_S = 10
LOCKOUT_S = 5.0
OVERTIME_FACTOR = 1.4
VALID_SLEEPY = (0, 100)
LOG_MAX = 128  # D-02 attack-session bound: diagnostic log is capped so
               # sustained rate-limit abuse cannot grow the heap unbounded
               # (target failure mode was reset -> OEM_PASS; still safe-side,
               # now bounded by construction). Most-recent entries kept.


class Side(Enum):
    LEFT = "left"
    RIGHT = "right"


class State(Enum):
    OEM_PASS = "OEM_PASS"   # de-energized bypass, OEM switches live
    IDLE = "IDLE"           # energized, holding, awaiting command
    MOVING = "MOVING"
    FAULT = "FAULT"


@dataclass
class Headlight:
    position: float = 0.0  # 0 down .. 100 up
    moving: bool = False
    last_travel_ms: int = 0


@dataclass
class Controller:
    """Tick-based (dt seconds) deterministic model. No wall clock, no I/O."""
    left: Headlight = field(default_factory=Headlight)
    right: Headlight = field(default_factory=Headlight)
    state: State = State.OEM_PASS
    oem_path_ok: bool = True   # True when de-energized bypass conducts
    bonded: bool = False
    ble_connected: bool = False
    time_s: float = 0.0
    _wdt_last_feed: float = 0.0
    _cmd_times: list = field(default_factory=list)
    _lockout_until: float = 0.0
    _invalid_streak: int = 0
    fault: str = ""
    wave_delay_s: float = 0.35
    sleepy: dict = field(default_factory=lambda: {"left": 50.0, "right": 50.0})
    _queue: list = field(default_factory=list)
    _active: dict = field(default_factory=lambda: {"side": None, "target": 0.0, "t": 0.0, "dur": 0.0})
    log: list = field(default_factory=list)
    # ---- OEM retractor multi-press (physical, works without BLE) ----
    press_gap_s: float = 1.0            # configurable 0.2–2.0 s
    button_map: dict = field(default_factory=lambda: {2: "wink:L", 3: "wink:R", 4: "wave:B"})
    headlamps_on: bool = False
    allow_with_lamps_on: bool = False  # bypass-headlights-on option, default OFF
    _press_count: int = 0
    _press_timer: float = 0.0
    # ---- auxiliary buttons (physical, works without BLE) ----
    aux_map: dict = field(default_factory=lambda: {"aux1": "up:B", "aux2": "wave:B"})
    aux_loop: dict = field(default_factory=lambda: {"aux1": False, "aux2": False})
    _loop: dict = field(default_factory=lambda: {"aux": None, "action": ""})

    def _log(self, msg: str):
        self.log.append(msg)
        if len(self.log) > LOG_MAX:
            del self.log[:len(self.log) - LOG_MAX]

    # ---- lifecycle ----
    def boot(self):
        self.state = State.OEM_PASS
        self.oem_path_ok = True
        self.left.moving = self.right.moving = False
        self._active = {"side": None, "target": 0.0, "t": 0.0, "dur": 0.0}
        self._queue.clear()
        self.fault = ""
        self._wdt_last_feed = self.time_s
        self._press_count = 0
        self._press_timer = 0.0
        self._loop = {"aux": None, "action": ""}
        self._log("boot->OEM_PASS")

    def power_loss(self):
        self.boot()
        self.ble_connected = False
        self._log("power_loss->OEM_PASS")

    def feed_watchdog(self):
        self._wdt_last_feed = self.time_s

    # ---- auth / BLE ----
    def pair(self, pin_ok: bool):
        if pin_ok:
            self.bonded = True
            self._log("paired")
            return True
        return False

    def ble_connect(self, bonded_device: bool):
        self.ble_connected = True
        self._bonded_link = bonded_device
        if self.state == State.OEM_PASS:
            self.state = State.IDLE
        self._log(f"ble_connect bonded={bonded_device}")

    def ble_disconnect(self):
        self.ble_connected = False
        self._queue.clear()  # cancel macro at quantum boundary
        self._loop = {"aux": None, "action": ""}  # looping aux macro cancelled
        if self._active["side"] is None:
            self.state = State.IDLE if not self.fault else State.FAULT
        self._log("ble_disconnect->cancel_queue")

    def _auth_ok(self, authenticated: bool) -> bool:
        if not (self.ble_connected and getattr(self, "_bonded_link", False)
                and self.bonded and authenticated):
            self._invalid_streak += 1
            if self._invalid_streak >= 5:
                self._lockout_until = self.time_s + LOCKOUT_S
            return False
        return True

    def _rate_ok(self) -> bool:
        if self.time_s < self._lockout_until:
            return False
        self._cmd_times = [t for t in self._cmd_times if self.time_s - t < 1.0]
        if len(self._cmd_times) >= RATE_LIMIT_PER_S:
            self._lockout_until = self.time_s + LOCKOUT_S
            return False
        self._cmd_times.append(self.time_s)
        return True

    # ---- commands ----
    def set_sleepy(self, side: str, value: float, authenticated: bool = True) -> bool:
        if not self._auth_ok(authenticated) or not self._rate_ok():
            return False
        lo, hi = VALID_SLEEPY
        if not (lo <= value <= hi):
            self._invalid_streak += 1
            return False
        self._invalid_streak = 0
        self.sleepy[side] = float(value)
        return True

    def command(self, action: str, authenticated: bool = True) -> bool:
        """action in: up/down/wink/blink/wave/sleepy/stop/clear, with :L/:R/:B suffix."""
        if self.state == State.FAULT and action != "clear":
            return False
        if not self._auth_ok(authenticated):
            return False
        # NOTE: rate limiting is enforced once inside _dispatch; checking here
        # as well would consume two tokens per command (audit fix 2026-09-22).
        ok = self._dispatch(action, count_invalid=True)
        if ok:
            self._invalid_streak = 0
        return ok

    def _dispatch(self, action: str, count_invalid: bool = False) -> bool:
        """Queue motion without BLE auth (physical-button path). Rate-limited."""
        if self.state == State.FAULT and action != "clear":
            return False
        if not self._rate_ok():
            return False
        base, _, arg = action.partition(":")
        # Wave direction: :LR = left-first, :RL = right-first (BLE_PROTOCOL).
        # Unknown/empty side defaults to both, left-first.
        targets = {"L": ["left"], "R": ["right"], "RL": ["right", "left"]}.get(
            arg, ["left", "right"])
        if base == "stop":
            self._queue.clear()
            self._loop = {"aux": None, "action": ""}
            self._active = {"side": None, "target": 0.0, "t": 0.0, "dur": 0.0}
            self.left.moving = self.right.moving = False
            self.state = State.IDLE
            return True
        if base == "clear":
            self.fault = ""
            self.state = State.IDLE
            return True
        plan = self._expand(base, targets)
        if plan is None:
            if count_invalid:
                self._invalid_streak += 1
                if self._invalid_streak >= 5:
                    self._lockout_until = self.time_s + LOCKOUT_S
            return False
        # stagger: enqueue with 150 ms gaps (interlock, never parallel inrush)
        t = 0.0
        for side, target in plan:
            self._queue.append({"side": side, "target": target, "delay": t})
            t += STAGGER_S
        if self.state in (State.OEM_PASS, State.IDLE):
            self.state = State.MOVING
        return True

    def _expand(self, base: str, targets: list) -> list | None:
        hl = {"left": self.left, "right": self.right}
        if base == "up":
            return [(s, 100.0) for s in targets]
        if base == "down":
            return [(s, 0.0) for s in targets]
        if base == "sleepy":
            return [(s, self.sleepy[s]) for s in targets]
        if base == "wink":
            out = []
            for s in targets:
                out += [(s, 100.0), (s, 0.0)] if hl[s].position > 50 else [(s, 0.0), (s, 100.0)]
            return out
        if base == "blink":
            return [(s, 0.0 if hl[s].position > 50 else 100.0) for s in targets]
        if base == "wave":
            seq = ["left", "right"] if targets == ["left", "right"] else targets
            return [(s, 0.0 if hl[s].position > 50 else 100.0) for s in seq]
        return None

    # ---- OEM retractor multi-press (F5) ----
    def configure_press_gap(self, ms: int, authenticated: bool = True) -> bool:
        if not self._auth_ok(authenticated) or not self._rate_ok():
            return False
        if not (200 <= ms <= 2000):
            return False
        self.press_gap_s = ms / 1000.0
        return True

    def map_button(self, presses: int, action: str, authenticated: bool = True) -> bool:
        if not self._auth_ok(authenticated) or not self._rate_ok():
            return False
        if not (2 <= presses <= 9):
            return False
        base, _, arg = action.partition(":")
        if self._expand(base, {"L": ["left"], "R": ["right"]}.get(arg, ["left", "right"])) is None \
                and base not in ("stop", "clear"):
            return False
        self.button_map[presses] = action
        return True

    def oem_press(self) -> str:
        """One physical OEM-retractor press. 1 press = stock (module idle);
        2–9 presses within gap window dispatch the mapped action."""
        if self.headlamps_on and not self.allow_with_lamps_on:
            self._press_count = 0
            self._press_timer = 0.0
            self._log("oem_press_ignored_lamps_on")
            return "ignored-lamps-on"
        self._press_count += 1
        self._press_timer = self.press_gap_s
        if self._press_count > 9:
            self._press_count = 0
            self._press_timer = 0.0
            self._log("oem_press_reset_overflow")
            return "reset-overflow"
        return f"count={self._press_count}"

    def _settle_presses(self):
        if self._press_count >= 2:
            action = self.button_map.get(self._press_count)
            if action is not None and self.state != State.FAULT:
                self._dispatch(action)
                self._log(f"oem_press_dispatched:{self._press_count}->{action}")
            else:
                self._log(f"oem_press_unmapped:{self._press_count}")
        self._press_count = 0
        self._press_timer = 0.0

    # ---- auxiliary buttons (F7: momentary/latching, looping with cancel) ----
    def configure_aux(self, aux: str, action: str, loop: bool,
                      authenticated: bool = True) -> bool:
        if aux not in ("aux1", "aux2"):
            return False
        if not self._auth_ok(authenticated) or not self._rate_ok():
            return False
        base, _, arg = action.partition(":")
        if self._expand(base, {"L": ["left"], "R": ["right"]}.get(arg, ["left", "right"])) is None \
                and base not in ("stop", "clear"):
            return False
        self.aux_map[aux] = action
        self.aux_loop[aux] = bool(loop)
        return True

    def aux_press(self, aux: str) -> str:
        """Physical aux press. Momentary: dispatch once. Latching-loop: toggle loop."""
        if aux not in ("aux1", "aux2"):
            return "invalid-aux"
        if self.state == State.FAULT:
            return "rejected-fault"
        if self._loop["aux"] == aux:
            self._loop = {"aux": None, "action": ""}
            self._queue.clear()
            self.state = State.IDLE
            self._log(f"aux_loop_cancelled:{aux}")
            return "loop-cancelled"
        action = self.aux_map[aux]
        if not self._dispatch(action):
            return "rejected-rate"
        if self.aux_loop[aux]:
            self._loop = {"aux": aux, "action": action}
            self._log(f"aux_loop_started:{aux}->{action}")
            return "loop-started"
        self._log(f"aux_momentary:{aux}->{action}")
        return "dispatched"

    def _pump_loop(self):
        if self._loop["aux"] is not None and not self._queue \
                and self._active["side"] is None and self.state == State.IDLE:
            self._dispatch(self._loop["action"])
            self._log(f"aux_loop_repeat:{self._loop['aux']}")

    # ---- time ----
    def tick(self, dt: float):
        self.time_s += dt
        # watchdog
        if self.time_s - self._wdt_last_feed > WDT_TIMEOUT_S:
            self.fault = "WDT_RESET"
            self.boot()
            self._log("wdt->OEM_PASS")
            return
        # OEM press-gap window expiry
        if self._press_timer > 0:
            self._press_timer -= dt
            if self._press_timer <= 0:
                self._settle_presses()
        # start next queued move when idle-side free
        if self._active["side"] is None and self._queue:
            nxt = self._queue[0]
            if nxt["delay"] > 0:
                nxt["delay"] -= dt
            else:
                self._queue.pop(0)
                side = nxt["side"]
                cur = self.left.position if side == "left" else self.right.position
                dur = abs(nxt["target"] - cur) / 100.0 * FULL_TRAVEL_S
                self._active = {"side": side, "target": nxt["target"], "t": 0.0, "dur": max(dur, 0.05)}
                (self.left if side == "left" else self.right).moving = True
                self.state = State.MOVING
        # progress active move
        a = self._active
        if a["side"] is not None:
            a["t"] += dt
            if a["t"] > max(a["dur"] * OVERTIME_FACTOR, MOTION_TIMEOUT_S):
                side = a["side"]
                (self.left if side == "left" else self.right).moving = False
                self._active = {"side": None, "target": 0.0, "t": 0.0, "dur": 0.0}
                self._queue.clear()
                self.fault = "OVERTIME_TRIP"
                self.state = State.FAULT
                self._log("overtime->FAULT")
                return
            if a["t"] >= a["dur"]:
                side = a["side"]
                hl = self.left if side == "left" else self.right
                hl.position = a["target"]
                hl.moving = False
                hl.last_travel_ms = int(a["dur"] * 1000)
                self._active = {"side": None, "target": 0.0, "t": 0.0, "dur": 0.0}
                if not self._queue:
                    self.state = State.IDLE
        self._pump_loop()
