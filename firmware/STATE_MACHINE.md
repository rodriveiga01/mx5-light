# STATE MACHINE

`OEM_PASS → IDLE → MOVING → IDLE`, with `FAULT` from MOVING on overtime and
`OEM_PASS` on boot / power-loss / WDT reset. `stop`/`clear`/BLE-loss return
toward IDLE (FAULT requires explicit `clear`).

- BOOT/power-loss/WDT: outputs de-energized (relay NC = OEM path), queue
  cleared, positions retained from NVS estimates, state OEM_PASS, reported.
- BLE connect (bonded): OEM_PASS → IDLE (OEM switches stay live in all states
  by hardware bypass availability).
- command(): IDLE/OEM_PASS → MOVING (queue ≥1 quantum); quanta execute with
  150 ms stagger; each quantum ≤5 s; overtime (>dur×1.4 and >5 s ceiling
  handling in model) → FAULT + de-energize + report.
- BLE disconnect: queue cleared; in-flight quantum finishes (≤5 s) then IDLE;
  never starts new motion unbonded. Looping aux macros cancelled.
- Invalid/unauthenticated: rejected, counted, lockout; motors never move.
- Concurrent L+R: queued, staggered — never parallel inrush by design.
- OEM retractor presses: counted in a configurable gap window (200–2000 ms,
  default 1000 ms); 1 press = stock (no module action); 2–9 presses settle on
  window expiry to the mapped action (unmapped counts and >9 ignored);
  presses while headlamps are ON are ignored unless the lamps-on bypass
  option is explicitly enabled. Physical path needs no BLE link.
- Aux buttons aux1/aux2: momentary dispatch, or latching loop while enabled;
  second press cancels; `stop`/BLE-loss/power-loss cancel the loop.
- No blocking delays in control path (tick-based; Arduino `loop()` must stay
  non-blocking on target — flagged for review).
