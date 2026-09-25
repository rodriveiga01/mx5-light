# Firmware model mapping — Python → C++ → test

Source of truth for CONTROL SEMANTICS: `firmware/model/headlight_controller.py`
(host-tested, 24 tests). Target implementation: `firmware/include/controller_core.h`
(C++17 header-only, no Arduino dependency) + glue in `firmware/src/main.cpp`.

## Constant map

| Python (`headlight_controller.py`) | C++ (`controller_core.h` / `config.h`) | Note |
|---|---|---|
| `FULL_TRAVEL_S = 3.2` | `kFullTravelS = 3.2f` / `FULL_TRAVEL_S` | PROPOSED default (A3), NOT measured |
| `MOTION_TIMEOUT_S = 5.0` | `kMotionTimeoutS` / `MOTION_TIMEOUT_MS 5000` | quantum ceiling |
| `WDT_TIMEOUT_S = 2.0` | `kWdtTimeoutS` / `WDT_TIMEOUT_MS 2000` | HW WDT on target, model WDT on host |
| `STAGGER_S = 0.15` | `kStaggerS` / `STAGGER_MS 150` | anti-inrush interlock |
| `RATE_LIMIT_PER_S = 10` | `kRateLimitPerS` / `RATE_LIMIT_PER_S` | single enforcement in `_dispatch`/`dispatch` |
| `LOCKOUT_S = 5.0` | `kLockoutS` / `LOCKOUT_S` | invalid-streak + flood |
| `OVERTIME_FACTOR = 1.4` | `kOvertimeFactor` | trip at `max(dur*1.4, 5 s)` |
| `VALID_SLEEPY = (0,100)` | `kValidSleepyLo/Hi`, `SLEEPY_MIN/MAX` | clamped |
| `State OEM_PASS/IDLE/MOVING/FAULT` | `nawink::State` identical | — |
| `press_gap_s 1.0 (200–2000 ms)` | `press_gap_s`, `configure_press_gap` | — |
| `button_map {2:wink:L,3:wink:R,4:wave:B}` | identical default | — |
| `aux_map/aux_loop` | identical defaults | — |

## Method map

| Python `Controller` | C++ `nawink::Controller` | Host test |
|---|---|---|
| `boot()` | `boot()` | `test_controller.py::test_boot_safe_no_motion`, `test_target_parity.py::test_boot_*` |
| `power_loss()` | `power_loss()` | reset/restore tests |
| `feed_watchdog()` | `feed_watchdog()` | WDT tests |
| `pair(pin_ok)` | `pair(bool)` | auth tests |
| `ble_connect(link)` | `ble_connect(bool)` | connect tests |
| `ble_disconnect()` | `ble_disconnect()` | BLE-loss tests |
| `_auth_ok/_rate_ok` | `auth_ok/rate_ok` | unauthenticated/rate/lockout tests |
| `set_sleepy` | `set_sleepy` | sleepy-bounds tests |
| `command` | `command` | every-command tests |
| `_dispatch` | `dispatch` | stagger/queue tests |
| `_expand` | `expand` | wink/wave LR/RL tests |
| `configure_press_gap/map_button` | same names | OEM-config tests |
| `oem_press/_settle_presses` | `oem_press/settle_presses` | OEM multi-press tests |
| `configure_aux/aux_press/_pump_loop` | same names | aux tests |
| `tick(dt)` | `tick(float)` | timeout/overtime/WDT/stagger tests |

## Target-only glue (`main.cpp`, NOT in Python model)

| Concern | Implementation | Verification status |
|---|---|---|
| GPIO init order pinMode→LOW | `safeBootPins()` + `relaysDeEnergized()` | DESIGN REVIEW ONLY — boot-glitch capture required (HV2). Host syntax-check proves order in source, not on silicon. |
| Relay polarity map | `applyRelayOutputs()` + `RELAY_POLARITY_INVERT` | PROPOSED (A1). Bench HV2 decides. Single correction point. |
| HW watchdog 2 s | `esp_task_wdt_init/add/reset` (`#ifdef ARDUINO`) | NOT VERIFIED without ESP32 hardware. Model WDT is tested; target WDT is code-reviewed only. |
| NimBLE GATT + bonding | `setupBle()`, `MotionCallbacks`, `PasskeyCallbacks` | NOT VERIFIED over the air. Parser (`handleBleCommand`) mirrors tested `command`/`set_sleepy`. |
| NVS persistence | `persistence.h` load/save + version key | LOGIC REVIEWED. On-target NVS behavior UNVERIFIED. Host invariant (no auto-motion after load) IS tested. |
| Non-blocking loop | 10 ms tick, no `delay()` | REVIEWED in source. Timing accuracy UNVERIFIED on target. |
| Diagnostics/version | `FW_VERSION 0.2.1`, `SETTINGS_VERSION 2`, fault codes | Version string cross-checked app-side (see app tests). |

## Drift policy

Python model and C++ core MUST stay semantically identical. Any behavior
change requires updating BOTH + the mapped test. `firmware/test_host_cpp/`
compiles the REAL `controller_core.h` with `g++` and executes assertions —
a logic test of the target header, not a GPIO proof.
