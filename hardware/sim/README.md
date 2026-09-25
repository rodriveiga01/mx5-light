# Circuit simulation — status + hand analysis (NO SPICE results claimed)

`ngspice` is NOT installed in this environment AND no validated motor/TVS/buck
SPICE models are vendored. Policy: NEVER invent simulation results.
`scripts/verify.sh` therefore records `SKIPPED: ngspice circuit sim` — honest,
not a pass.

## What IS provided here (design review, not simulation)

- `tvs-entry-notes.md` — TVS + reverse-block + fuse hand-analysis (paper).
- `buck-ldo-notes.md` — buck/LDO cascade hand-analysis (paper).
- `relay-coil-notes.md` — coil suppression hand-analysis (paper).
- `protection-frontend.cir.BLOCKED` — ngspice deck TEMPLATE, suffixed
  `.BLOCKED` so no tooling mistakes it for a run deck. It is NOT runnable
  until measured motor parameters + vendor TVS/buck models are added.
- `RESULTS.md` — explicitly NO RESULTS (lists the blocking measurements).

Motor electrical parameters are UNKNOWN (A1: topology, stall/inrush, R/L,
limit behavior all UNMEASURED). No motor model is provided here by design —
inventing one would be fabrication. The deck stays BLOCKED until bench HV2/HV3
measures the genuine article.

## To enable (maintainer machine, AFTER bench measurements)

1. Measure: motor DC resistance + inductance + stall/inrush current + full-travel
   time on a fused jig (HV2/HV3). Record in `docs/HARDWARE_VALIDATION.md`.
2. Vendor: TVS SPICE model (Littelfuse), buck transient model (TI), relay coil
   L/R. Save under `hardware/sim/models/` with source URLs + dates.
3. Rename `protection-frontend.cir.BLOCKED` → `.cir`, fill measured values,
   run `ngspice protection-frontend.cir`, save raw + plots + report under
   `hardware/exports/`, update RESULTS.md with real numbers.
4. Re-run `scripts/verify.sh` — ngspice check remains SKIPPED-by-policy until
   a validated deck + models exist; do NOT force it to PASS.
