# Power-entry protection notes

Proposed path: J1 → 10 A fuse → reverse-battery block → protected 12 V rail.
A bidirectional SMBJ26CA TVS clamps the rail; place it near the connector.
The TVS is for fast transients, not sustained load dump.

- Published SMBJ26CA clamp: 42.1 V at 14.3 A (10/1000 µs); this does not
  establish protection against longer automotive pulses.
- Confirm reverse-block limits, fuse coordination, and component ratings
  against datasheets and bench tests before vehicle use.
- The 10 A fuse protects the module spur; motors remain on the OEM power path.

No SPICE result is claimed. A sustained transient may damage the TVS; test
protection on a fused, current-limited bench setup.
