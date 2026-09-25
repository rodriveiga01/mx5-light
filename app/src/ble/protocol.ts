// SPDX-License-Identifier: GPL-3.0-or-later
// BLE protocol mirror — UUID values adapted from OpenWink constants.h (GPLv3)
// for interop; re-expressed here. Must match firmware/include/ble_uuids.h.
export const UUIDS = {
  WINK_SERVICE: "a144c6b0-5e1a-4460-bb92-3674b2f51520",
  OTA_SERVICE: "e24c13d7-d7c7-4301-903a-7750b09fc935", // RESERVED local-only
  SETTINGS_SERVICE: "cb5f7a1f-59f2-418e-b9d1-d6fc5c85a749",
  HEADLIGHT: "034a383c-d3e4-4501-b7a5-1c950db4f3c7",
  BUSY: "8d2b7b9f-c6a3-4f56-9f4f-2dc7d7873c18",
  LEFT_STATUS: "c4907f4a-fb0c-440c-bbf1-4836b0636478",
  RIGHT_STATUS: "784dd553-d837-4027-9143-280cb035163a",
  SLEEPY_EYE: "a8237fed-e0a4-4ecd-9881-9b5dbb3f5902",
  SYNC: "eceed349-998f-46a2-9835-4f2db7552381",
  CUSTOM_COMMAND: "1313c33f-e793-422c-8c04-c82be9fe8a02",
  PASSKEY: "f61146f2-791d-4ef7-95aa-b565097f69c2",
  UNPAIR: "c67c4fd1-21ce-4a75-bd16-629f990e575d",
  RESET: "a55946b8-1978-4522-8a29-27d17e21b092",
  HEADLIGHT_BYPASS: "ada2537e-0399-4d2a-9eab-0c7cb60d3500",
  SWAP_ORIENTATION: "3ddd922d-14ca-4785-9cd0-39a530e8b14d",
} as const;

export const FW_VERSION_EXPECTED = "0.2.1";

const CMDS = new Set(["up","down","wink","blink","wave","sleepy","stop","clear"]);
const SIDES = new Set(["L","R","B","LR","RL"]);

export function buildCommand(base: string, side = "B"): string {
  if (!CMDS.has(base)) throw new Error(`invalid-command:${base}`);
  const s = base === "wave" ? (side === "R" ? "RL" : "LR") : side;
  if (!SIDES.has(s)) throw new Error(`invalid-side:${s}`);
  return s === "B" || base === "stop" || base === "clear" ? base + (base==="stop"||base==="clear"?"":":B") : `${base}:${s}`;
}

export function clampSleepy(v: number): number {
  if (!Number.isFinite(v) || v < 0 || v > 100) throw new Error(`sleepy-out-of-range:${v}`);
  return Math.round(v);
}

export function encodeSleepy(left: number, right: number): string {
  return `L=${clampSleepy(left)},R=${clampSleepy(right)}`;
}

export function decodeSleepy(s: string): { left: number; right: number } {
  const m = /^L=(\d+),R=(\d+)$/.exec(s.trim());
  if (!m) throw new Error(`malformed-sleepy:${s}`);
  return { left: clampSleepy(Number(m[1])), right: clampSleepy(Number(m[2])) };
}

export function validatePasskey(pin: string): boolean {
  return /^\d{6}$/.test(pin);
}

export function clampWaveDelayMs(ms: number): number {
  if (!Number.isFinite(ms) || ms < 100 || ms > 1500)
    throw new Error(`wave-delay-out-of-range:${ms}`);
  return Math.round(ms);
}

export function clampPressGapMs(ms: number): number {
  if (!Number.isFinite(ms) || ms < 200 || ms > 2000)
    throw new Error(`press-gap-out-of-range:${ms}`);
  return Math.round(ms);
}

export type ConnState = "idle" | "scanning" | "pairing" | "connected" | "lost" | "locked";

export function nextConnState(
  cur: ConnState,
  ev: "scan" | "found" | "paired" | "drop" | "lockout" | "forget" | "reconnect"
): ConnState {
  switch (cur) {
    case "idle": return ev === "scan" ? "scanning" : cur;
    case "scanning": return ev === "found" ? "pairing" : ev === "forget" ? "idle" : cur;
    case "pairing": return ev === "paired" ? "connected" : ev === "forget" ? "idle" : cur;
    case "connected":
      if (ev === "drop") return "lost";
      if (ev === "lockout") return "locked";
      if (ev === "forget") return "idle";
      return cur;
    case "lost": return ev === "reconnect" ? "connected" : ev === "forget" ? "idle" : cur;
    case "locked": return ev === "reconnect" ? "connected" : ev === "forget" ? "idle" : cur;
    default: return cur;
  }
}

export function faultLabel(code: string): string {
  if (code === "OVERTIME_TRIP") return "overtime";
  if (code === "WDT_RESET") return "watchdog";
  if (code === "LOCKOUT") return "locked";
  if (!code || code === "-") return "none";
  return "unknown";
}

// Offline-first: no network calls in this module by design.
export const LOCAL_ONLY = true;
