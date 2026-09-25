# HARNESS — plug-and-play adapter (no cutting / no splicing)

Principle: the module NEVER cuts factory wiring. Two adapter legs insert
between the factory headlight-motor connectors and the motors:

- Leg A (vehicle side, J1): 8-pos sealed plug mates to factory retractor
  harness branches (VBAT feed, GND, L/R OEM pairs, BTN_SENSE tap, AUX return).
- Leg B (motor sides, J2/J3): 3-pos sealed plugs mate to each retractor
  motor pigtail. Relay commons face the motors; NC faces the car.

De-energized relays conduct OEM_x pins straight to MOT_x pins, so with the
module unpowered, in reset, or with the MCU held in reset, the car behaves
exactly as stock. Verified in `simulator/` (oem_path_ok); hardware-level
continuity + functional OEM pass-through with module unpowered is validation
case HV1 in `docs/HARDWARE_VALIDATION.md` (NOT RUN — no PCB built).

## Conductors (PROPOSED — meter on donor car before crimping; OE-02 PARTIALLY RESOLVED rev 0.3.2)

No 2.5/1.5 mm² conductor is EVER crimped into an MX120G terminal (PROHIBITED —
family max 1.0 mm² per Molex). Every power/motor position uses a 1.0 mm²
FLRY-B pigtail at the shell (terminal grip per SD-36799-001 at order),
joined to the main run by the specified transition below. Run gauges are
UNCHANGED (motor current UNKNOWN — sizing TBD on measured stall, OE-02).

| Leg | Wire | Section | Color intent | Notes |
|---|---|---|---|---|
| A | VBAT | 2.5 mm² run + 1.0 mm² pigtail at J1 | red | fused 10 A on module; route with existing loom; transition per §Gauge transition (OE-02) |
| A | GND | 2.5 mm² run + 1.0 mm² pigtail at J1 | black | star point to chassis ground bolt; ring terminal on run side; transition per §Gauge transition |
| A | OEM_L_A / OEM_L_B | 1.5 mm² run + 1.0 mm² pigtail at J1 | match factory R/Y + B where confirmed | twisted pair, < 60 cm; transition per §Gauge transition |
| A | OEM_R_A / OEM_R_B | 1.5 mm² run + 1.0 mm² pigtail at J1 | match factory | twisted pair, < 60 cm; transition per §Gauge transition |
| A | BTN_SENSE | 0.5 mm² | yellow | high-Z opto tap only; fused 0.5 A inline; FITS 0.35–1.0 mm² terminal range (no pigtail needed) |
| B | MOT_x_A / MOT_x_B | 1.5 mm² run + 1.0 mm² pigtail at J2/J3 | match motor pigtail | relay common to motor; strain-relieved; transition per §Gauge transition |
| B | SHIELD/GND drain | 0.5 mm² | bare + sleeve | drain at module end only; FITS terminal range (no pigtail needed) |

## Gauge transition (OE-02 specified method — mechanical solution, electrical sizing of runs TBD)

Method: ultrasonic splice + adhesive-lined heat-shrink seal (TE SCT-NO.1:
expanded 7.6 mm / recovered 1.7 mm, −40…+150 °C, automotive splice sealing
per TE catalog 1654296-3). 10 joints total: 2× (2.5↔1.0) + 8× (1.5↔1.0).
Joints staggered along the leg (never one fat station), as close as practical
to the shell, inside corrugated loom. 100% pull-test every joint, values
recorded (acceptance per engineer at review; pigtail crimps meet Molex
PS-36790-001 pullout minimums). Supporting arithmetic (CALCULATED FROM
DATASHEETS): 1.0 mm² FLRY-B guidance capacity 19 A (Eland) vs 10 A F1 — the
pigtail is thermally protected by the module fuse with margin (guidance
value; bundling/ambient derating at review); pigtail drop at 1 A over
10 cm ≈ 2×0.1 m×18.5 mΩ/m ≈ 3.7 mV — negligible. Module-side pigtail current
is coils + MCU only (<1 A); motor current flows in the RUNS, whose sizing
stays TBD on measured stall (OE-02 gate above). Same-range catalog butt connectors evaluated
and REJECTED for these mixed pairs (3M Scotchlok ranges 0.34–1.0 / 1.5–2.5 /
4–6 cover same-range joints only — neither range covers a 1.0↔1.5 or 1.0↔2.5
pair). Insulation OD vs seal cavity verified at order (SD-36799-001).

## Connectors + mating (PROPOSED candidates — fit NOT verified)

| Ref | Module side | Mating side | Terminal family | Sealing | Status |
|---|---|---|---|---|---|
| J1 | MX120G-series 8-pos plug (sealed) | MX120G-series 8-pos receptacle + 36799-series terminals + cavity plugs | MX120G terminals (0.35–1.0 mm² family max per Molex page + PS-36790-001, verified 2026-09-24; 1.0 mm² grip P/N per SD-36799-001 at order), manufacturer crimp tool | Sealed shell + boots | PROPOSED — PARTIALLY RESOLVED (OE-02): shell P/N + keying NEEDS-MEASUREMENT on donor car; 1.0 mm² pigtails at shell + transition per §Gauge transition; direct 2.5/1.5 crimp PROHIBITED |
| J2 | MX120G-series 3-pos (LEFT motor) | Matching 3-pos + terminals | Same family | Sealed | PROPOSED — NOT fit-verified |
| J3 | MX120G-series 3-pos (RIGHT motor) | Matching 3-pos + terminals | Same family | Sealed | PROPOSED — NOT fit-verified |
| J_PROG | 1x06 P2.54 header | Dupont jumper (bench only) | — | Unsealed (bench) | PROPOSED (bench use only) |
| GND bolt | Ring terminal M6 | Chassis ground bolt (star point) | Automotive ring 2.5 mm² | — | PROPOSED — torque per car manual |

No connector compatibility is claimed until manufacturer-datasheet review +
physical mating on the donor car (see REAL_WORLD_CHECKLIST J).

## Harness diagram (logical — NOT a measured car pinout)

```text
CAR SWITCH SIDE (metered, battery OFF)          MODULE                    MOTORS
-------------------------------                 ------                    ------
RETRACT-feed (R/Y intent*) ----+---> J1.1 VBAT -> F1 -> D_REV -> TVS -> buck/LDO -> MCU
                               |     J1.2 GND ---> star -> chassis bolt
OEM_L pair (metered) ----------+---> J1.3 OEM_L_A --+-- K1.NC -- K1.COM --> J2.1 MOT_L_A --> motor L
                               +---> J1.4 OEM_L_B --+-- K2.NC -- K2.COM --> J2.2 MOT_L_B --> motor L
OEM_R pair (metered) ----------+---> J1.5 OEM_R_A --+-- K3.NC -- K3.COM --> J3.1 MOT_R_A --> motor R
                               +---> J1.6 OEM_R_B --+-- K4.NC -- K4.COM --> J3.2 MOT_R_B --> motor R
Retractor-switch tap ---------+-----------------> J1.7 BTN_SENSE (opto high-Z, 0.5A inline) -> GPIO9
Aux return (RESERVED) --------+-----------------> J1.8 AUX_GND (reserved, unconnected at module in rev 0.3.2;
                                          NO aux signal pins exist — aux buttons DEFERRED, see below)
                                         MCU drive (energized ONLY on command):
                                           GPIO4/5 --Q1/Q2--> K1/K2 coils --NO--> polarity drive
                                           GPIO6/7 --Q3/Q4--> K3/K4 coils --NO--> polarity drive
* Wire colors/pinouts are manual-derived and MUST be metered per car (COMPATIBILITY.md).
  De-energized: NC conducts OEM straight through (stock behavior, HV1).
  Energized: NO takes control (staggered >=150 ms, both-high inhibited).
  Aux buttons: NOT wired in rev 0.3.2 — AUX1/AUX2 (GPIO14/21 + 10 k pull-ups)
  have no connector pins and J1.8 AUX_GND has no module-side return, so no aux
  button can be physically connected (netlist-verified). Aux control logic is
  host-verified in firmware/core for a future connector rev; physical aux
  inputs are DEFERRED (OE-16). Do NOT promise aux buttons in user docs.
```

Strain relief: corrugated loom + boots at J1/J2/J3, gland entry at enclosure
(+X face, 16 mm), internal strain posts, drip loop with connectors DOWN.
Fuse location: F1 on module + holder accessible WITHOUT opening the case
(service disconnect). Routing: with factory loom, away from heat/sharp
edges/fan belt, <60 cm twisted OEM pairs, service loop, grommet at sheet
metal. Retention: latch click + 30 N / 60 s pull (HV7).

## Build rules

1. Crimp (never solder) terminals with the manufacturer tool; pull-test
   every crimp. Solder wicks into strands and fractures under vibration.
2. Sealed shells + cavity plugs on unused positions; dielectric grease on
   seals for the engine-bay location.
3. Corrugated loom + heat-shrink boots at both shells; anchor with cable
   ties to the factory loom — no contact with hot or moving parts.
4. Label every leg (L / R / CAR / MOTOR) with wrap-around markers.
5. Removal = unplug adapters, re-mate factory connectors. No trace remains.

## Verification (bench, before any car contact)

- HV1: with module unpowered, measure < 1 Ω OEM_x → MOT_x per pole and
  confirm stock-equivalent switching through the adapter with a
  current-limited supply + simulated load (criteria in HARDWARE_VALIDATION.md).
- OE-02 gate: run-gauge sizing (2.5/1.5 mm² against measured stall/inrush +
  voltage-drop calc) must be recorded BEFORE vehicle contact; the transition
  method above is the specified mechanical solution. No 2.5/1.5 mm² conductor
  enters an MX120G 0.35–1.0 mm² terminal — EVER (prohibition stands).
- Insulation: > 10 MΩ pin-to-shell at 100 V DC (bench meter).
- All results in `docs/HARDWARE_VALIDATION.md`. Any car-side pinout that
  cannot be meter-verified is marked NOT RUN — never assumed.
