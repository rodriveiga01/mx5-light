# KiCad design

Sources: `na-wink.kicad_sch` (schematic) and `na-wink.kicad_pcb` (layout
draft). This is a review draft, not fabrication-ready.

- ERC: 0 errors. DRC: 22 unconnected GND items because CLI checks do not fill zones.
- Library remapping is required. Relay footprints are placeholders and must be corrected.
- Connector fit, motor current, relay rating, and relay polarity remain unverified.
- Do not order a board until layout checks and bench validation HV1–HV4 pass.

Reports: `hardware/exports/erc.rpt` and `hardware/exports/drc.rpt`.
