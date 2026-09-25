# TVS + reverse-block + fuse — hand analysis (paper, NOT simulation)

Topology (schematic): J1 VBAT → F1 (10 A ATO) → D_REV ideal-diode block
(LM5050-1 + CSD18532 class) → node `12V_PROTECTED` → TVS1 SMBJ26CA bidirectional to GND →
buck VIN. Star GND at supply. TVS placed at connector (shortest loop).

## Intent (review reasoning, no numbers invented)

- Normal 9–16 V operating: ideal-diode drop is small by design (MOSFET Rds(on)
  intent, NOT a Schottky 0.4 V); verify against LM5050-1 + CSD18532 datasheets
  at order. Fuse F1 sees load current only (coils + MCU, NOT motors — motors
  stay on the OEM 30 A domain through relay contacts).
- Reverse battery (-12 V / -24 V jump-start reversal intent): ideal-diode gate
  drive holds the MOSFET OFF, blocking reverse conduction. Residual risk: gate
  survival at -30 V must be confirmed in the datasheet + HV4 lab test.
- Positive transient (load-dump intent): TVS1 clamps at its Vc; bulk 470 µF
  50 V rides through short dips. NUMBERS (verified 2026-09-24, rev 0.3.2):
  SMBJ26CA Vrwm 26 V / Vbr 28.9–31.9 V @ 1 mA / Vc 42.1 V @ Ipp 14.3 A
  (600 W @10/1000 µs); LM46002-Q1 operating max 60 V / abs max 65 V
  (SNVSAA2B §6.1). Vc now coordinates WITH MARGIN (see PROTECTION_ANALYSIS
  §OE-01). SMBJ-class remains an ESD/fast-transient device, NOT a long-dump
  absorber (Test A: up to 101 V / 400 ms) — unsuppressed dump is
  sacrificial-safe (TVS short → F1 clears) + LAB. The 600 W rating assumes
  10/1000 µs ONLY — NO ISO-pulse energy coverage is implied.
- Fuse coordination: 10 A module vs 30 A HEAD/RETRACT upstream. The module fuse
  MUST clear first on a module-side short without taking the OEM fuses. Time-
  current curves: review at order, prove at HV4 (intentional spur short on a
  FUSED SPUR ONLY, current-limited supply).

## Why no SPICE here

A TVS + MOSFET + fuse transient sim without vendor models is fiction. The
`.cir.BLOCKED` template lists the exact nodes; fill it only after vendor
models + measured load steps exist. Until then this note + HV4 lab test are
the evidence (both honestly labeled).
