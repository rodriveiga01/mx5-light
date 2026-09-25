# Relay coil notes

Four 5 V relay coils use NPN low-side drivers and flyback diodes. GPIO LOW
should leave each relay de-energized on the OEM path; confirm boot behavior
with a scope before vehicle use.

The proposed 560 Ω base resistor improves drive margin over 1 kΩ, but coil
current and transistor saturation must be checked against the final relay
datasheet. Relay and motor inrush behavior is unmeasured. Firmware staggers
commands by at least 150 ms; this does not replace hardware validation.

No SPICE result is claimed. Confirm coil data and suppression on the bench.
