# KiCad sources — review draft (NOT fabrication-ready)

Canonical sources:

- `na-wink.kicad_sch` — full review schematic: power entry (J1→F1→TVS→buck→LDO),
  ESP32-S3-WROOM-1, 4× DPDT relay pairs (K1/K2 left, K3/K4 right, NC=OEM),
  4× NPN coil drivers + flyback, opto BTN_SENSE, J2/J3 motor connectors,
  programming header, bulk/decoupling, test points, all safety nets labeled.
- `na-wink.kicad_pcb` — routed placement draft: 100×70 mm outline, M3 holes,
  protection/MCU/relay/connector zones, 2.0 mm motor tracks (necked jogs
  noted), 1.5/1.0 mm power, 0.6 mm 5V backbone (via-channel limited),
  0.5/0.3 mm signal, 3V3 + OEM/motor returns on B.Cu with vias, B.Cu GND
  pour zone, silkscreen + comments.

## Status (honest)

- ERC (kicad-cli 10.0.6, 2026-09-23): 0 errors — remaining warnings are
  off-grid/stub/library-remap classes (see hardware/erc_drc/STATUS.txt).
  Report: hardware/exports/erc.rpt (genuine tool output).
- DRC (kicad-cli 10.0.6, 2026-09-23): 0 shorting/clearance/crossing errors;
  22 unconnected_items, ALL on GND pads/vias (kicad-cli does not fill zones
  -- the pour fills at fab/GUI time). Report:
  hardware/exports/drc.rpt (genuine tool output).
- Symbols/footprints use INTENT names (`na-wink:*`, `Connector_Molex:MX120G-*`,
  `Relay_THT:Omron_G2R-2`, `Module:ESP32-S3-WROOM-1`, …). They REQUIRE
  remapping to the KiCad stock libraries + project libraries on first open.
  NOTE: relay footprints are SMD placeholders — the real G2R-2-DC5 is an
  8-pin through-hole part. Must be corrected before any fab order.
- Vehicle-side shells/terminals are PROPOSED candidates (PINOUT.md
  NEEDS-MEASUREMENT). Do NOT order shells from this draft.
- Relay contact rating vs measured stall (A1): TBD at bench HV3. Do NOT downrate.
- Motor polarity map (`RELAY_POLARITY_INVERT` in firmware) is decided at HV2.

## How to continue (maintainer machine, KiCad ≥8)

```sh
# open + remap
kicad hardware/kicad/na-wink.kicad_sch   # resolve missing symbols via stock Device/power/Connector libs

# checks (save reports under hardware/exports/)
kicad-cli sch erc --output hardware/exports/erc.rpt hardware/kicad/na-wink.kicad_sch
kicad-cli pcb drc --output hardware/exports/drc.rpt hardware/kicad/na-wink.kicad_pcb
```

ERC/DRC MUST be clean + bench HV1–HV4 MUST all succeed before any fab order
(do not order — NOT RUN here; see hardware/exports/MANIFEST.txt).
`hardware/exports/MANIFEST.txt` refuses fab until then.

## Netlist (for review)

See `hardware/exports/netlist-review.csv` (cross-check against the
schematic labels before relying on it).
