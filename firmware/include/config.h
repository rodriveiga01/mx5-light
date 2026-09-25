// SPDX-License-Identifier: GPL-3.0-or-later
// NA Wink — ESP32-S3 target configuration (single source of truth for pins
// and timing on target; host model mirrors these values).
//
// GPIO 4-7 are NOT strapping pins on ESP32-S3 (safe for relay drive).
// NC = OEM path. GPIO LOW = coil OFF = OEM bypass (NPN low-side).
// All timing values are PROPOSED defaults (assumption A3) — NOT measured.
// See docs/RESEARCH.md, docs/VERIFICATION_AUDIT.md.
#pragma once

#ifndef FW_VERSION
#define FW_VERSION "0.2.1"
#endif
#define FW_MODEL_MAP "firmware/model/headlight_controller.py -> controller_core.h (see MAPPING.md)"

// ---- Relay drive GPIOs (PROPOSED — verify boot glitch on first article, HV2) ----
#define RELAY_L_POL1_GPIO 4
#define RELAY_L_POL2_GPIO 5
#define RELAY_R_POL1_GPIO 6
#define RELAY_R_POL2_GPIO 7

// ---- Sense / aux inputs (active-low, internal pull-up; PROPOSED) ----
#define OEM_SENSE_GPIO 9    // via 4N25-class opto, high-Z tap (see PINOUT.md)
#define AUX1_GPIO 14
#define AUX2_GPIO 21

// ---- Timing (seconds / ms; PROPOSED defaults, NOT measured) ----
#define FULL_TRAVEL_S 3.2f
#define MOTION_TIMEOUT_MS 5000
#define WDT_TIMEOUT_MS 2000
#define STAGGER_MS 150
#define OVERTIME_FACTOR 1.4f
#define WAVE_DELAY_DEFAULT_MS 350
#define WAVE_DELAY_MIN_MS 100
#define WAVE_DELAY_MAX_MS 1500

// ---- BLE policy ----
#define RATE_LIMIT_PER_S 10
#define LOCKOUT_S 5
#define PAIRING_WINDOW_S 300
#define INVALID_STREAK_LOCKOUT 5

// ---- Sleepy-eye bounds ----
#define SLEEPY_MIN 0
#define SLEEPY_MAX 100

// ---- NVS keys ----
#define NVS_NS "nawink"
#define NVS_KEY_SLEEPY_L "sleepy_l"
#define NVS_KEY_SLEEPY_R "sleepy_r"
#define NVS_KEY_WAVE_MS "wave_ms"
#define NVS_KEY_GAP_MS "gap_ms"
#define NVS_KEY_SETTINGS_VER "cfg_ver"
#define SETTINGS_VERSION 2
