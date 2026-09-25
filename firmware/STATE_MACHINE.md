# State machine

`OEM_PASS → IDLE → MOVING → IDLE`. Overtime → `FAULT` (needs `clear`).
Boot / power-loss / WDT reset → outputs de-energized (relays rest on the
OEM bypass), queue cleared, state `OEM_PASS`.

- Commands queue with 150 ms stagger, never parallel inrush. `stop` cancels.
- BLE disconnect cancels the queue; the in-flight move finishes, then IDLE.
- Bad/unauthenticated commands are rejected + counted, motors never move.
- OEM retractor presses: 1 = stock, 2–9 = mapped action after the gap window.
- Aux buttons: momentary or latching loop; second press (or `stop`) cancels.
