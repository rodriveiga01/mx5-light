# Model mapping (Python ↔ C++)

Source of truth: `firmware/model/headlight_controller.py`. The target
`firmware/include/controller_core.h` mirrors it method-for-method
(`boot`, `command`, `dispatch`, `expand`, `tick`, `oem_press`, aux…).
Rule: change behavior in BOTH, plus tests. `test_host_cpp/` compiles the
real C++ header with `g++` and runs assertions (logic only, no GPIO proof).

Key constants (PROPOSED defaults, NOT measured): full travel 3.2 s,
overtime trip at max(dur×1.4, 5 s), WDT 2 s, stagger 150 ms, rate ≤10/s,
lockout 5 s, sleepy 0–100. Target-only glue (`main.cpp`: GPIO order,
relay polarity, HW watchdog, NimBLE, NVS) is code-reviewed only —
unverified without ESP32 hardware and bench scope captures (HV2).
