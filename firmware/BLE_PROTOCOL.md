# BLE protocol (FW 0.2.1)

- Pair with 6-digit passkey → bonded. Motion writes need bonded link + fresh auth, else rejected (5 bad tries → 5 s lockout).
- Commands: `up/down/wink/blink:L|R|B`, `wave:LR|RL`, `sleepy:L|R|B`, `stop`, `clear`, `SLEEPY_EYE "L=nn,R=nn"`. Max 10/s, 150 ms stagger, `stop` cancels.
- Status: busy flag, L/R position + moving, fault codes (`OVERTIME_TRIP`, `WDT_RESET`, `LOCKOUT`). Boots to `OEM_PASS`, never moves on its own.
- Pairing is casual-abuse protection only, not MITM-proof. Unpair before handing the module to anyone.
