# BLE PROTOCOL (v0.2.1, local-first; FW_VERSION 0.2.1 in `ble_uuids.h`)

GATT services/characteristics: see `firmware/include/ble_uuids.h` (UUIDs
adapted from OpenWink for interop). All motion writes require bonded link +
fresh auth flag; else rejected + counted (5 consecutive → 5 s lockout).

## Pairing / association

- First boot (or after UNPAIR): 5-min discoverable window, name `Wink-<serial>`.
- Client writes 6-digit passkey to PASSKEY char → bonded. Only bonded central
  may write motion/config chars; others get read-only status.
- UNPAIR clears bond on module; app must "Forget module" too (both sides).
- No cloud, no account, no OTA server. OTA service UUIDs RESERVED.

## Commands (HEADLIGHT / SLEEPY_EYE / CUSTOM_COMMAND, UTF-8 short strings)

`up:L|R|B  down:L|R|B  wink:L|R|B  blink:L|R|B  wave:LR|RL  sleepy:L|R|B
stop  clear` + `SLEEPY_EYE "L=nn,R=nn"` (0–100 clamped). Invalid → reject.
Rate ≤10/s; wave/wink expand to queued quanta with 150 ms stagger; `stop`
cancels queue; BLE disconnect cancels at quantum boundary (never mid-drive).

## Status (notify/read)

BUSY (0/1), LEFT/RIGHT_STATUS (0–100 + moving flag), SYNC (settings hash),
movement timing ms per side. FAULT codes: `OVERTIME_TRIP`, `WDT_RESET`,
`LOCKOUT`. Boot always reports `OEM_PASS` first; no motion without command.

## Security limits

Passkey+bonding+rate-limit deter casual abuse only. Legacy BLE pairing is
not strong MITM protection — documented in USER_GUIDE; keep pairing window
closed, unpair before transfer, never actuate near roads/crowds.
