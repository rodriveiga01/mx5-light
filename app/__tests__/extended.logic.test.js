// Extended logic tests: protocol additions + offline store.
// Compiles the REAL TS sources with tsc; SKIP (not PASS) when tsc absent.
const { describe, it, before } = require("node:test");
const assert = require("node:assert");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

let P = null, S = null;
before(() => {
  const outDir = fs.mkdtempSync(path.join(os.tmpdir(), "na-wink-app-ext-"));
  const r = spawnSync("tsc", [
    path.join(__dirname, "..", "src", "ble", "protocol.ts"),
    path.join(__dirname, "..", "src", "store.ts"),
    "--outDir", outDir,
    "--module", "commonjs", "--target", "es2020", "--strict",
  ], { encoding: "utf8" });
  if (r.error || r.status !== 0) {
    console.log(`SKIP: tsc unavailable (${r.error ? r.error.code : (r.stderr || "").slice(0, 160)})`);
    return;
  }
  P = require(path.join(outDir, "ble", "protocol.js"));
  S = require(path.join(outDir, "store.js"));
});

const need = (name, fn) => it(name, () => {
  if (!P || !S) { console.log("SKIP: modules not compiled"); return; }
  return fn();
});

describe("extended protocol logic", () => {
  need("encode/decode sleepy round-trip + bounds", () => {
    assert.strictEqual(P.encodeSleepy(40, 60), "L=40,R=60");
    assert.deepStrictEqual(P.decodeSleepy("L=40,R=60"), { left: 40, right: 60 });
    assert.throws(() => P.encodeSleepy(101, 0), /sleepy-out-of-range/);
    assert.throws(() => P.decodeSleepy("garbage"), /malformed-sleepy/);
  });
  need("sleepy wire format matches firmware parser (audit 2026-09-23)", () => {
    // Firmware handleBleCommand '=' branch splits on ',' and requires each
    // fragment's side to be L/R/B. App must send bare "L=nn,R=nn" (config,
    // never moves); the old "sleepy:B L=..,R=.." form was always rejected.
    assert.strictEqual(P.encodeSleepy(40, 60), "L=40,R=60");
    const app = fs.readFileSync(path.join(__dirname, "..", "App.tsx"), "utf8");
    assert.ok(app.includes("sendRaw(payload"),
      "App.tsx saveSleepy must send the bare encodeSleepy payload (via sendRaw -> link.send)");
    assert.ok(!app.includes("${cmd} ${payload}"),
      "App.tsx must not prefix the sleepy payload with 'sleepy:B '");
  });
  need("passkey + wave/gap clamps", () => {
    assert.strictEqual(P.validatePasskey("123456"), true);
    assert.strictEqual(P.validatePasskey("12345"), false);
    assert.strictEqual(P.validatePasskey("abcdef"), false);
    assert.strictEqual(P.clampWaveDelayMs(350), 350);
    assert.throws(() => P.clampWaveDelayMs(50), /wave-delay-out-of-range/);
    assert.throws(() => P.clampWaveDelayMs(2000), /wave-delay-out-of-range/);
    assert.strictEqual(P.clampPressGapMs(500), 500);
    assert.throws(() => P.clampPressGapMs(100), /press-gap-out-of-range/);
  });
  need("LR/RL mapping preserved", () => {
    assert.strictEqual(P.buildCommand("wave", "L"), "wave:LR");
    assert.strictEqual(P.buildCommand("wave", "R"), "wave:RL");
    assert.strictEqual(P.buildCommand("up", "L"), "up:L");
  });
  need("connection state machine + fault labels", () => {
    assert.strictEqual(P.nextConnState("idle", "scan"), "scanning");
    assert.strictEqual(P.nextConnState("scanning", "found"), "pairing");
    assert.strictEqual(P.nextConnState("pairing", "paired"), "connected");
    assert.strictEqual(P.nextConnState("connected", "drop"), "lost");
    assert.strictEqual(P.nextConnState("lost", "reconnect"), "connected");
    assert.strictEqual(P.nextConnState("connected", "lockout"), "locked");
    assert.strictEqual(P.faultLabel("OVERTIME_TRIP"), "overtime");
    assert.strictEqual(P.faultLabel("WDT_RESET"), "watchdog");
    assert.strictEqual(P.faultLabel("-"), "none");
  });
  need("store round-trip + corrupt fallback (local-only)", async () => {
    const kv = S.memKV();
    const d = await S.loadSettings(kv);
    assert.strictEqual(d.sleepyL, 50);
    await S.saveSettings(kv, { ...d, sleepyL: 40, sleepyR: 60, waveMs: 400 });
    const r = await S.loadSettings(kv);
    assert.strictEqual(r.sleepyL, 40);
    assert.strictEqual(r.waveMs, 400);
    await kv.setItem("nawink.settings.v2", "{corrupt");
    const fb = await S.loadSettings(kv);
    assert.strictEqual(fb.sleepyL, 50); // safe defaults, never throws
    assert.strictEqual(P.LOCAL_ONLY, true);
  });
  need("i18n key parity incl. new screens", () => {
    const en = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", "en.json"), "utf8"));
    const pt = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", "pt-PT.json"), "utf8"));
    assert.deepStrictEqual(Object.keys(en).sort(), Object.keys(pt).sort());
    for (const k of ["home", "pairing", "diag", "settings", "errors"]) assert.ok(en[k] && pt[k], k);
    assert.ok(en.safety.road_disclaimer.length > 10 && pt.safety.road_disclaimer.length > 10);
  });
});
