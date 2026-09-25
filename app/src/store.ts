// SPDX-License-Identifier: GPL-3.0-or-later
// Local settings store (offline-first). Persistence backend is injected so
// host tests run with an in-memory map while the device build uses
// AsyncStorage (see App.tsx wiring). No network, no telemetry.
export interface Settings {
  sleepyL: number;
  sleepyR: number;
  waveMs: number;
  gapMs: number;
  buttonMap: Record<number, string>;
  version: number;
}

export const DEFAULTS: Settings = {
  sleepyL: 50,
  sleepyR: 50,
  waveMs: 350,
  gapMs: 1000,
  buttonMap: { 2: "wink:L", 3: "wink:R", 4: "wave:B" },
  version: 2,
};

export interface KV {
  getItem(k: string): Promise<string | null>;
  setItem(k: string, v: string): Promise<void>;
  removeItem(k: string): Promise<void>;
}

export function memKV(): KV {
  const m = new Map<string, string>();
  return {
    getItem: async (k) => (m.has(k) ? m.get(k)! : null),
    setItem: async (k, v) => { m.set(k, v); },
    removeItem: async (k) => { m.delete(k); },
  };
}

const KEY = "nawink.settings.v2";

function clamp(n: number, lo: number, hi: number, fb: number): number {
  return Number.isFinite(n) && n >= lo && n <= hi ? Math.round(n) : fb;
}

export async function loadSettings(kv: KV): Promise<Settings> {
  try {
    const raw = await kv.getItem(KEY);
    if (!raw) return { ...DEFAULTS };
    const p = JSON.parse(raw);
    if (p.version !== 2) return { ...DEFAULTS }; // version mismatch -> safe defaults
    return {
      sleepyL: clamp(p.sleepyL, 0, 100, 50),
      sleepyR: clamp(p.sleepyR, 0, 100, 50),
      waveMs: clamp(p.waveMs, 100, 1500, 350),
      gapMs: clamp(p.gapMs, 200, 2000, 1000),
      buttonMap: p.buttonMap ?? { ...DEFAULTS.buttonMap },
      version: 2,
    };
  } catch {
    return { ...DEFAULTS }; // corrupt storage -> safe defaults, never throws
  }
}

export async function saveSettings(kv: KV, s: Settings): Promise<void> {
  const clean: Settings = {
    sleepyL: clamp(s.sleepyL, 0, 100, 50),
    sleepyR: clamp(s.sleepyR, 0, 100, 50),
    waveMs: clamp(s.waveMs, 100, 1500, 350),
    gapMs: clamp(s.gapMs, 200, 2000, 1000),
    buttonMap: s.buttonMap,
    version: 2,
  };
  await kv.setItem(KEY, JSON.stringify(clean));
}
