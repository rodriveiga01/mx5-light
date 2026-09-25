# Buck + LDO cascade — hand analysis (paper, NOT simulation)

Topology: `12V_PROTECTED` → U_BUCK (LM46002-Q1 automotive, 12→5 V, 2 A;
rev 0.3.2 OE-01 resolution — was LMR33630-class) → `5V` (relay coils + LDO
in) → U_LDO (KF33BD-class, 5→3.3 V, 1 A) → `3V3` (MCU). Bulk 470 µF 50 V
(uprated: 42.1 V clamp vs 35 V rating) at buck in/out intent; 100 nF per
rail + per MCU pin. Inductor + compensation per LM46002-Q1 datasheet at
bench; precision-enable UVLO programmed at bring-up.

## Intent (review reasoning)

- Two-stage BECAUSE OpenWink rev1 proved single-stage 12→3.3 LDO overheats
  (lesson applied, not repeated). 5→3.3 drop is small → LDO dissipation is
  small by design; confirm at HV6 soak (60 °C, 30 min, 3V3 in spec).
- Buck inductor + compensation per the buck datasheet reference design — NOT
  improvised. Saturation current must exceed peak (coil inrush + MCU TX bursts)
  with margin; verify at order + HV2 load steps.
- Relay coils on 5V (NOT 3V3) so coil switching does not yank the MCU rail;
  flyback diodes D1-D4 return coil energy locally. Scope 3V3 during switching
  at HV2.
- Standby target <5 mA (deep-sleep intent, HV5 NOT RUN). No standby number is
  claimed here.

## Why no SPICE here

No vendor buck transient model is vendored; a behavioral guess would prove
nothing about stability or thermal. Bench HV2/HV6 are the evidence gates.
