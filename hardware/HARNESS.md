# Harness (plug-and-play, never cut factory wiring)

Two adapter legs sit between the factory connectors and the motors. Relays
de-energized = OEM straight through, so unpowered the car behaves stock
(HV1 validation, NOT RUN).

## Wires (PROPOSED — meter on the donor car before crimping)

| Leg | Wire | Size | Note |
|---|---|---|---|
| A (car) | VBAT | 2.5 mm² + 1.0 pigtail | red, fused 10 A on module |
| A | GND | 2.5 mm² + 1.0 pigtail | black, chassis star bolt |
| A | OEM_L_A/B, OEM_R_A/B | 1.5 mm² + 1.0 pigtail | twisted pairs, < 60 cm |
| A | BTN_SENSE | 0.5 mm² | yellow, opto tap only, 0.5 A inline fuse |
| B (motors) | MOT_x_A/B | 1.5 mm² + 1.0 pigtail | relay common to motor |
| B | SHIELD/GND | 0.5 mm² | drain at module end only |

Rule: NEVER crimp 2.5/1.5 mm² into an MX120G terminal (family max 1.0 mm²).
Join runs to pigtails with ultrasonic splice + adhesive heat-shrink, 10 joints.

## Connectors (candidates — fit NOT verified, meter before mating)

| Ref | Module side | Mating side |
|---|---|---|
| J1 | MX120G 8-pos sealed plug | 8-pos receptacle + terminals + cavity plugs |
| J2/J3 | MX120G 3-pos | matching 3-pos + terminals |
| J_PROG | 1x06 P2.54 header | bench jumpers only |

## Build rules

1. Crimp with the manufacturer tool, pull-test every crimp. Never solder terminals.
2. Sealed shells + cavity plugs on empty positions; loom + boots on both shells.
3. Label every leg (L / R / CAR / MOTOR). Removal = unplug, no trace remains.
4. Verify unpowered: < 1 Ω OEM→MOT per pole, > 10 MΩ pin-to-shell.
