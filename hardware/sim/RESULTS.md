# Simulation RESULTS — none (explicitly)

No ngspice run has been performed. No transient/thermal numbers are claimed.
Behavioral evidence lives in `simulator/` (deterministic Python, motor-free)
and host tests — NEITHER models analog behavior.

Blocking measurements (all NOT RUN):

1. Motor DC R/L + stall/inrush current + travel time (HV2/HV3, genuine NA motor).
2. Vendor TVS SPICE model (Littelfuse SMBJ26CA bidirectional or engineer-chosen alternate per OE-01).
3. Vendor buck transient model (TI LM46002-Q1 or chosen alternate).
4. Relay coil L/R + contact rating confirmation (G2R-2-DC5 datasheet review; 5A@30VDC resistive verified in catalog — DC inductive motor-load rating TBD at HV3).
5. Fuse time-current coordination (10 A module vs 30 A upstream).

When 1–5 exist: unblock the `.cir.BLOCKED` deck, run ngspice, save raw + plots
+ dated report under `hardware/exports/`, and update this file with real numbers.
