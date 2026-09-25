// SPDX-License-Identifier: GPL-3.0-or-later
// NA Wink — settings persistence abstraction.
//
// Target (ESP32-S3, Arduino): Preferences (NVS) with version key.
// Host syntax-check / unit path: in-memory stub (no I/O).
//
// Persisted (all require explicit Save; NEVER auto-move on load):
//   sleepy L/R (0..100), wave delay ms (100..1500), press-gap ms (200..2000),
//   settings version. Positions are estimates only — boot NEVER animates.
#pragma once

#ifdef ARDUINO
#include <Preferences.h>
#include "config.h"

namespace nawink {
struct Settings {
  float sleepy_l = 50.0f;
  float sleepy_r = 50.0f;
  int wave_ms = WAVE_DELAY_DEFAULT_MS;
  int gap_ms = 1000;
};

inline Settings loadSettings() {
  Settings s;
  Preferences p;
  if (!p.begin(NVS_NS, true)) return s;
  int ver = p.getInt(NVS_KEY_SETTINGS_VER, 0);
  if (ver != SETTINGS_VERSION) {
    p.end();
    return s;  // version mismatch -> safe defaults, no motion
  }
  s.sleepy_l = p.getFloat(NVS_KEY_SLEEPY_L, 50.0f);
  s.sleepy_r = p.getFloat(NVS_KEY_SLEEPY_R, 50.0f);
  s.wave_ms = p.getInt(NVS_KEY_WAVE_MS, WAVE_DELAY_DEFAULT_MS);
  s.gap_ms = p.getInt(NVS_KEY_GAP_MS, 1000);
  p.end();
  // Clamp defensively (NVS corruption must never produce out-of-range motion)
  if (!(0.0f <= s.sleepy_l && s.sleepy_l <= 100.0f)) s.sleepy_l = 50.0f;
  if (!(0.0f <= s.sleepy_r && s.sleepy_r <= 100.0f)) s.sleepy_r = 50.0f;
  if (!(WAVE_DELAY_MIN_MS <= s.wave_ms && s.wave_ms <= WAVE_DELAY_MAX_MS))
    s.wave_ms = WAVE_DELAY_DEFAULT_MS;
  if (!(200 <= s.gap_ms && s.gap_ms <= 2000)) s.gap_ms = 1000;
  return s;
}

inline void saveSettings(const Settings& s) {
  Preferences p;
  if (!p.begin(NVS_NS, false)) return;
  p.putFloat(NVS_KEY_SLEEPY_L, s.sleepy_l);
  p.putFloat(NVS_KEY_SLEEPY_R, s.sleepy_r);
  p.putInt(NVS_KEY_WAVE_MS, s.wave_ms);
  p.putInt(NVS_KEY_GAP_MS, s.gap_ms);
  p.putInt(NVS_KEY_SETTINGS_VER, SETTINGS_VERSION);
  p.end();
}
}  // namespace nawink

#else  // ---- host / syntax-check path: in-memory only ----

namespace nawink {
struct Settings {
  float sleepy_l = 50.0f;
  float sleepy_r = 50.0f;
  int wave_ms = 350;
  int gap_ms = 1000;
};
inline Settings loadSettings() { return Settings{}; }
inline void saveSettings(const Settings&) {}
}  // namespace nawink

#endif
