# PINOUT — module connectors (PROPOSED, NEEDS-MEASUREMENT)

All shells/terminals below are CANDIDATES. None is fit-verified on a donor
car. Meter every conductor before mating. NC = de-energized OEM path.

## J1 — vehicle switch side (8-pos sealed, MX120G-class intent)

| Pin | Signal | From | Note |
|---|---|---|---|
| 1 | VBAT (RETRACT 30A domain) | car R/L feed | fused 10 A on module |
| 2 | GND | chassis B | star point |
| 3 | OEM_L_A | retractor relay path L | relay NC pole |
| 4 | OEM_L_B | retractor relay path L | relay NC pole |
| 5 | OEM_R_A | retractor relay path R | relay NC pole |
| 6 | OEM_R_B | retractor relay path R | relay NC pole |
| 7 | BTN_SENSE | retractor switch tap (opto) | high-Z only |
| 8 | AUX_GND (RESERVED) | future aux-button return — unconnected at module in rev 0.3.2 (no aux signal pins; DEFERRED per OE-16) | do NOT wire buttons to this pin in this rev |

## J2/J3 — motor sides (to each retractor motor)

| Pin | Signal | Note |
|---|---|---|
| 1 | MOT_x_A | relay common (NC=OEM_x_A, NO=drive) |
| 2 | MOT_x_B | relay common (NC=OEM_x_B, NO=drive) |
| 3 | SHIELD/GND | drain + pull check |

Test points: TP_VBAT, TP_3V3, TP_COIL_L/R, TP_MOT_L/R, TP_GND near J1.
Review checklist (manual): pin numbering vs shell keying, NC/NO orientation,
coil polarity + flyback, TVS placement at entry, fuse accessibility.
