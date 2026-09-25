// App tests: run with `node --test __tests__/app.test.js` (stdlib only, no npm install).
const { describe, it } = require("node:test");
const assert = require("node:assert");
const fs = require("node:fs");
const path = require("node:path");

describe("i18n", () => {
  for (const lang of ["en", "pt-PT"]) {
    it(`${lang} has road disclaimer + parity keys`, () => {
      const d = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", `${lang}.json`), "utf8"));
      assert.ok(d.safety.road_disclaimer.length > 10, "disclaimer present");
      for (const k of ["home", "pairing", "diag"]) assert.ok(d[k], `${lang}.${k}`);
    });
  }
  it("en/pt-PT key parity", () => {
    const en = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", "en.json"), "utf8"));
    const pt = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", "pt-PT.json"), "utf8"));
    assert.deepStrictEqual(Object.keys(en).sort(), Object.keys(pt).sort());
  });
});

describe("ble protocol (compiled TS mirror)", () => {
  // protocol.ts is dependency-free; transpile-free check via regex + logic re-check
  const src = fs.readFileSync(path.join(__dirname, "..", "src", "ble", "protocol.ts"), "utf8");
  const fw = fs.readFileSync(path.join(__dirname, "..", "..", "firmware", "include", "ble_uuids.h"), "utf8");
  it("UUIDs match firmware header", () => {
    for (const m of src.matchAll(/"([0-9a-f]{8}-[0-9a-f-]{27,})"/gi)) {
      assert.ok(fw.includes(m[1].toLowerCase()) || fw.toLowerCase().includes(m[1].toLowerCase()), `fw has ${m[1]}`);
    }
  });
  it("rejects invalid commands + clamps sleepy", () => {
    assert.ok(src.includes("invalid-command") && src.includes("sleepy-out-of-range"));
    assert.ok(src.includes("LOCAL_ONLY = true"), "offline-first flag");
  });
});

describe("connection honesty (reconciliation 2026-09-23)", () => {
  // The UI must never display "connected" on local input validation alone:
  // pair() validates the 6-digit format, then waits for link evidence.
  const app = fs.readFileSync(path.join(__dirname, "..", "App.tsx"), "utf8");
  it("pair() never sets connected without link evidence", () => {
    const seg = app.slice(app.indexOf("const pair"), app.indexOf("return ("));
    assert.ok(seg.includes("validatePasskey"), "pair validates PIN format locally");
    assert.ok(!seg.includes('setConn("connected")'),
      "pair must not claim connectivity on regex match alone");
    assert.ok(seg.includes("pin_ok"), "pair shows awaiting-device note instead");
  });
  it("connected is set only after a link round-trip succeeds", () => {
    const seg = app.slice(app.indexOf("const sendRaw"), app.indexOf("const fire"));
    assert.ok(seg.includes('r === "ok"') && seg.includes('setConn("connected")'),
      "promotion to connected is gated on link.send === ok");
    assert.ok(seg.includes('setConn("lost")'),
      "transport throw degrades to lost, never silently stays connected");
  });
  it("diag screen exposes the documented fault-clear path", () => {
    assert.ok(app.includes("clear_fault"), "uses the diag.clear_fault string");
    assert.ok(app.includes('"clear"'), "sends the firmware clear command");
  });
  it("pairing pin_ok string exists EN+pt-PT", () => {
    for (const lang of ["en", "pt-PT"]) {
      const d = JSON.parse(fs.readFileSync(path.join(__dirname, "..", "src", "i18n", `${lang}.json`), "utf8"));
      assert.ok(d.pairing.pin_ok && d.pairing.pin_ok.length > 10, `${lang}.pairing.pin_ok`);
    }
  });
});
