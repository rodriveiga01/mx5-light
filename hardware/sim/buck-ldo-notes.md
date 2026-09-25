# Buck and LDO notes

Proposed supply: protected 12 V → 5 V buck (LM46002-Q1) → 3.3 V LDO
(KF33BD-class). Relay coils use 5 V; the MCU uses 3.3 V. Values and parts
must be checked against datasheets before build.

- Use the buck datasheet design for inductor and compensation; verify load
  steps and 3.3 V stability on the bench.
- Check 3.3 V temperature and regulation during a 60 °C, 30-minute soak.
- Standby target is under 5 mA; not measured.

No SPICE result is claimed. Vendor transient models and bench measurements
are not available.
