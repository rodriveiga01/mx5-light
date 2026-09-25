# Circuit notes

No SPICE run or analog results are available. Motor parameters and validated
vendor models are missing; `protection-frontend.cir.BLOCKED` is a template,
not a runnable simulation.

- `RESULTS.md` — missing measurements and evidence status.
- `tvs-entry-notes.md`, `buck-ldo-notes.md`, `relay-coil-notes.md` — design
  notes only; bench checks are still required.

Enable simulation only after measuring the real motor and adding validated
TVS, buck, and relay models. Save actual results under `hardware/exports/`.
