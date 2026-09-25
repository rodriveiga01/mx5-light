import React, { useState } from "react";
import { Button, Text, TextInput, View } from "react-native";
// SPDX-License-Identifier: GPL-3.0-or-later
// NA Wink — offline-first Expo app (EN + pt-PT). Local BLE only: no cloud
// accounts, no telemetry, no OTA server. BLE transport is injected via the
// `Link` prop so host tests exercise the UI state machine without a phone or
// module; device builds provide the NimBLE transport (see BUILD.md —
// configured-only here, needs EAS/Xcode/SDK for binaries).
import en from "./src/i18n/en.json";
import pt from "./src/i18n/pt-PT.json";
import { buildCommand, encodeSleepy, validatePasskey, FW_VERSION_EXPECTED } from "./src/ble/protocol";

export interface Link {
  send(cmd: string): Promise<"ok" | "rejected">;
  readStatus(): Promise<{ left: number; right: number; fault: string }>;
}

type Lang = "en" | "pt-PT";
type Screen = "home" | "pair" | "diag" | "settings";

const STR: Record<Lang, any> = { en, "pt-PT": pt };

export default function App(props: { link?: Link; lang?: Lang }) {
  const lang: Lang = props.lang ?? "en";
  const t = STR[lang];
  const [screen, setScreen] = useState<Screen>("home");
  const [conn, setConn] = useState<"idle" | "connected" | "lost">("idle");
  const [pin, setPin] = useState("");
  const [sleepL, setSleepL] = useState("50");
  const [sleepR, setSleepR] = useState("50");
  const [waveMs, setWaveMs] = useState("350");
  const [note, setNote] = useState("");
  const link: Link =
    props.link ?? {
      send: async () => "rejected",
      readStatus: async () => ({ left: 0, right: 0, fault: "-" }),
    };

  // Transport-confirmed connectivity ONLY: "connected" is set exclusively
  // after a link round-trip succeeds ("ok"). Local PIN/format validation
  // NEVER sets it (reconciliation fix 2026-09-23: pair() used to display
  // "Connected" on regex match alone, with no BLE exchange). Transport
  // failure (throw) means the device is lost; "rejected" keeps prior state.
  const sendRaw = async (cmd: string, okNote: string) => {
    try {
      const r = await link.send(cmd);
      if (r === "ok") {
        setConn("connected");
        setNote(okNote);
      } else {
        setNote(t.errors.rejected);
      }
    } catch {
      setConn("lost");
      setNote(t.errors.rejected);
    }
  };

  const fire = async (base: string, side = "B") => {
    let cmd: string;
    try {
      cmd = buildCommand(base, side);
    } catch {
      setNote(t.errors.invalid);
      return;
    }
    await sendRaw(cmd, cmd);
  };

  const saveSleepy = async () => {
    let payload: string;
    try {
      // Wire format per firmware/BLE_PROTOCOL.md SLEEPY_EYE: bare
      // "L=nn,R=nn" (config write, never moves motors). The "sleepy:B"
      // motion command is separate (firmware dispatch path). Do NOT prefix
      // the payload: handleBleCommand in main.cpp rejects fragments whose
      // side is not L/R/B (audit fix 2026-09-23: "sleepy:B L=..,R=.." was
      // always rejected, so Save could never succeed against real firmware).
      payload = encodeSleepy(Number(sleepL), Number(sleepR));
    } catch {
      setNote(t.errors.invalid);
      return;
    }
    await sendRaw(payload, t.home.save);
  };

  const pair = async () => {
    if (!validatePasskey(pin)) {
      setNote(t.pairing.invalid_pin);
      return;
    }
    // PIN format is valid LOCALLY — this proves nothing about the radio link,
    // so conn stays as-is (idle/lost) until a link round-trip succeeds via
    // sendRaw. The user completes pairing on the module, then any successful
    // command promotes the display to connected.
    setNote(t.pairing.pin_ok);
  };

  return (
    <View>
      <Text>{t.safety.road_disclaimer}</Text>
      <Text>
        {t.app.name} v{FW_VERSION_EXPECTED} — {conn === "connected" ? t.home.connected : t.home.disconnected}
      </Text>
      <View>
        <Button title={t.home.title} onPress={() => setScreen("home")} />
        <Button title={t.pairing.title} onPress={() => setScreen("pair")} />
        <Button title={t.diag.title} onPress={() => setScreen("diag")} />
        <Button title={t.settings.title} onPress={() => setScreen("settings")} />
      </View>
      {screen === "home" && (
        <View>
          <Text>
            {t.home.left}/{t.home.right}/{t.home.both}
          </Text>
          <Button title={`${t.home.up} L`} onPress={() => fire("up", "L")} />
          <Button title={`${t.home.up} R`} onPress={() => fire("up", "R")} />
          <Button title={`${t.home.down} B`} onPress={() => fire("down", "B")} />
          <Button title={`${t.home.wink} L`} onPress={() => fire("wink", "L")} />
          <Button title={`${t.home.wink} R`} onPress={() => fire("wink", "R")} />
          <Button title={t.home.wave} onPress={() => fire("wave", "L")} />
          <Button title={t.home.stop} onPress={() => fire("stop")} />
          <Text>
            {t.home.sleepy_left}: {sleepL} {t.home.sleepy_right}: {sleepR}
          </Text>
          <Button title={t.home.save} onPress={saveSleepy} />
          <Text>
            {t.home.wave_delay}: {waveMs}
          </Text>
        </View>
      )}
      {screen === "pair" && (
        <View>
          <Text>{t.pairing.window}</Text>
          <Text>{t.pairing.passkey}</Text>
          <TextInput value={pin} onChangeText={setPin} keyboardType="numeric" />
          <Button title={t.pairing.connect} onPress={pair} />
          <Button
            title={t.pairing.forget}
            onPress={() => {
              setConn("idle");
              setPin("");
            }}
          />
        </View>
      )}
      {screen === "diag" && (
        <DiagScreen t={t} link={link} onClear={() => sendRaw("clear", t.diag.clear_fault)} />
      )}
      {screen === "settings" && (
        <View>
          <Text>{t.settings.buttons}</Text>
          <Text>
            {t.settings.oem_presses}: 2=wink:L 3=wink:R 4=wave:B
          </Text>
          <Text>
            {t.settings.press_gap}: 200–2000
          </Text>
          <Text>{t.settings.aux}: aux1=up:B aux2=wave:B</Text>
          <Text>{t.settings.lamps_guard}</Text>
          <Text>
            {t.diag.firmware}: {FW_VERSION_EXPECTED}
          </Text>
        </View>
      )}
      <Text>{note}</Text>
    </View>
  );
}

function DiagScreen({ t, link, onClear }: { t: any; link: Link; onClear: () => Promise<void> }) {
  const [s, setS] = useState("—");
  return (
    <View>
      <Text>
        {t.diag.position} / {t.diag.travel} / {t.diag.fault}
      </Text>
      <Button
        title={t.diag.title}
        onPress={async () => {
          try {
            const r = await link.readStatus();
            setS(`L=${r.left} R=${r.right} ${t.diag.fault}=${r.fault}`);
          } catch {
            setS(t.errors.offline);
          }
        }}
      />
      <Button title={t.diag.clear_fault} onPress={onClear} />
      <Text>{s}</Text>
    </View>
  );
}
