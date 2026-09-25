# Pinout (PROPOSED — meter every conductor before mating)

## J1 — car side (8-pos sealed)

| Pin | Signal | Note |
|---|---|---|
| 1 | VBAT | fused 10 A on module |
| 2 | GND | chassis star point |
| 3–4 | OEM_L_A / OEM_L_B | relay NC poles |
| 5–6 | OEM_R_A / OEM_R_B | relay NC poles |
| 7 | BTN_SENSE | opto tap, high-Z only |
| 8 | AUX_GND (RESERVED) | unconnected — do NOT wire buttons here |

## J2/J3 — motor sides

| Pin | Signal | Note |
|---|---|---|
| 1–2 | MOT_x_A / MOT_x_B | relay common (NC = OEM, NO = drive) |
| 3 | SHIELD/GND | drain |

NC = de-energized OEM path. Check: pin numbering vs shell keying, NC/NO
orientation, coil polarity + flyback direction, TVS at entry, fuse access.
