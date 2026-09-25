# Relay coil suppression — hand analysis (paper, NOT simulation)

Per coil (×4: K1-K4 via Q1-Q4): GPIO (3V3) → 560R base (D-07 rev 0.3.3,
was 1k) → MMBT2222A NPN low-side → coil to 5V, with 1N4148 flyback across
the coil (cathode to 5V side).

## Intent (review reasoning)

- GPIO LOW = transistor OFF = relay de-energized = NC/OEM (safe state).
  Verified in source order (`safeBootPins` → LOW first) + host logic tests.
  Boot-glitch capture still required at HV2 (scope all coil nodes at power-on).
- At turn-off the coil current recirculates through D1-D4 (flyback), NOT into
   the MCU pin. 1N4148 class is chosen for small-coil intent; confirm peak
   recirculation vs G2R-2-DC5 5 V-coil current at datasheet review (corrected
   2026-09-23: DC12 coil on the 5 V rail could never operate; exact 5V-coil
   current TBD from datasheet — re-verify 560R base-drive saturation margin,
   coil current intent <150 mA — verify, do NOT assume).
   D-07 analysis (eng/monte_carlo.py, rev 0.3.3): 1k gave Ib ~2.6 mA and a
   worst-corner (cold hFE + VOH droop) saturation margin of 0.80 — thin.
   560R gives Ib ~4.5 mA nominal, p01 margin 2.02, worst corner 1.42.
   Consequence of desaturation was mild (Vce ~1 V, coil still operates), but
   the fix is cheap and strictly improves margin; GPIO source ~4 mA is fine.
- Both-high inhibited in firmware (queued + staggered ≥150 ms); only one side
  is ever driven. Simultaneous L+R inrush is structurally avoided in logic;
  hardware inrush (both coils + one motor) is UNMEASURED until HV2/HV3.

## Why no SPICE here

Coil L/R are datasheet values (review at order); a sim without them adds no
knowledge beyond this note. The scope capture at HV2 is the evidence.
