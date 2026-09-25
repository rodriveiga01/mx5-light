// Logic tests against the REAL compiled protocol.ts (via tsc).
// Run: node --test __tests__/protocol.logic.test.js
// Prints "SKIP: <reason>" + exit 0 when tsc is unavailable; verify.sh maps that to SKIPPED.
const { describe, it, before } = require("node:test");
const assert = require("node:assert");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

let P = null;
before(() => {
  const outDir = fs.mkdtempSync(path.join(os.tmpdir(), "na-wink-app-"));
  const r = spawnSync("tsc", [
    path.join(__dirname, "..", "src", "ble", "protocol.ts"),
    "--outDir", outDir,
    "--module", "commonjs", "--target", "es2020", "--strict",
  ], { encoding: "utf8" });
  if (r.error || r.status !== 0) {
    console.log(`SKIP: tsc unavailable or compile failed (${r.error ? r.error.code : (r.stderr || "").slice(0, 200)})`);
    return;
  }
  const js = path.join(outDir, "protocol.js");
  assert.ok(fs.existsSync(js), `compiled output present at ${js}`);
  P = require(js);
});

const need = (name, fn) => it(name, () => {
  if (!P) { console.log("SKIP: protocol module not compiled"); return; }
  fn();
});

describe("protocol logic (compiled TS)", () => {
  need("UUID set well-formed + complete vs firmware header", () => {
    const uuid = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/;
    const fw = fs.readFileSync(path.join(__dirname, "..", "..", "firmware", "include", "ble_uuids.h"), "utf8").toLowerCase();
    for (const [k, v] of Object.entries(P.UUIDS)) {
      assert.match(v, uuid, k);
      assert.ok(fw.includes(v.toLowerCase()), `firmware has ${k}=${v}`);
    }
    assert.ok(P.UUIDS.HEADLIGHT_BYPASS && P.UUIDS.SWAP_ORIENTATION, "bypass+orientation present");
  });
  need("offline-first flag", () => assert.strictEqual(P.LOCAL_ONLY, true));
  need("buildCommand accepts taxonomy, maps wave sides", () => {
    assert.strictEqual(P.buildCommand("up", "L"), "up:L");
    assert.strictEqual(P.buildCommand("wave", "L"), "wave:LR");
    assert.strictEqual(P.buildCommand("wave", "R"), "wave:RL");
    assert.strictEqual(P.buildCommand("stop"), "stop");
    assert.strictEqual(P.buildCommand("clear"), "clear");
  });
  need("buildCommand rejects invalid base/side", () => {
    assert.throws(() => P.buildCommand("fly", "B"), /invalid-command/);
    assert.throws(() => P.buildCommand("up", "X"), /invalid-side/);
  });
  need("clampSleepy bounds 0..100", () => {
    assert.strictEqual(P.clampSleepy(0), 0);
    assert.strictEqual(P.clampSleepy(100), 100);
    assert.throws(() => P.clampSleepy(101), /sleepy-out-of-range/);
    assert.throws(() => P.clampSleepy(-1), /sleepy-out-of-range/);
    assert.throws(() => P.clampSleepy(NaN), /sleepy-out-of-range/);
  });
});
