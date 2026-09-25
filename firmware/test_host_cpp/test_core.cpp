// SPDX-License-Identifier: GPL-3.0-or-later
// Host execution test for the REAL target header firmware/include/controller_core.h.
// Compiles with system g++ (no Arduino, no ESP hardware) and RUNS assertions.
// Proves: target header LOGIC on host. Does NOT prove: GPIO, NimBLE, NVS, HW WDT.
#include <cassert>
#include <cstdio>
#include "controller_core.h"

using namespace nawink;

static void run(Controller& c, float s = 8.0f, float dt = 0.05f) {
  float t = 0;
  while (t < s) {
    c.feed_watchdog();
    c.tick(dt);
    t += dt;
  }
}
static Controller mk() {
  Controller c;
  c.boot();
  c.pair(true);
  c.ble_connect(true);
  c.feed_watchdog();
  return c;
}

int main() {
  int n = 0;
#define OK(cond, name) do { assert(cond); printf("ok %d - %s\n", ++n, name); } while (0)

  { Controller c; c.boot();
    OK(c.state == State::OEM_PASS, "boot->OEM_PASS");
    OK(c.oem_path_ok, "boot oem_path available");
    OK(!c.active.has() && c.queue.empty(), "boot no motion"); }
  { Controller c = mk(); c.command("up:B"); run(c);
    OK(c.left.position == 100.0f && c.right.position == 100.0f, "up:B both 100"); }
  { Controller c = mk(); c.command("up:B"); run(c); c.command("down:L"); run(c);
    OK(c.left.position == 0.0f && c.right.position == 100.0f, "independent L/R"); }
  { Controller c = mk();
    OK(c.set_sleepy("left", 50), "sleepy ok");
    OK(!c.set_sleepy("left", 101), "sleepy 101 rejected");
    OK(!c.set_sleepy("right", -1), "sleepy -1 rejected"); }
  { Controller c = mk();
    OK(!c.command("fly:B"), "invalid rejected");
    OK(c.queue.empty() && !c.active.has(), "invalid queues nothing"); }
  { Controller c = mk();
    for (int i = 0; i < 5; i++) { c.command("fly:B"); c.tick(0.01f); c.feed_watchdog(); }
    OK(!c.command("up:B"), "invalid-streak lockout arms"); }
  { Controller c = mk();
    for (int i = 0; i < 10; i++) { c.command("blink:B"); c.tick(0.01f); c.feed_watchdog(); }
    OK(!c.command("blink:B"), "rate budget 10/s"); }
  { Controller c = mk(); c.command("wave:RL");
    OK(c.queue.size() >= 2 && c.queue[0].side == "right", "wave:RL right-first"); }
  { Controller c = mk(); c.command("wave:B");
    OK(c.queue.size() >= 2 && c.queue[0].side == "left", "wave default left-first"); }
  { Controller c; c.boot(); c.ble_connect(false); c.feed_watchdog();
    OK(!c.command("up:B"), "unbonded rejected"); }
  { Controller c = mk(); c.command("wave:B"); c.tick(0.2f); c.ble_disconnect();
    OK(c.queue.empty(), "ble-loss cancels queue"); }
  { Controller c = mk(); c.command("up:B"); c.tick(3.0f);
    OK(c.state == State::OEM_PASS, "wdt->OEM_PASS"); }
  { Controller c = mk(); c.command("up:B"); run(c, 1.0f); c.power_loss();
    OK(c.state == State::OEM_PASS && c.oem_path_ok, "reset->OEM_PASS"); }
  { Controller c = mk();
    OK(c.set_sleepy("left", 40) && c.set_sleepy("right", 60), "sleepy set");
    c.power_loss(); run(c, 2.0f);
    OK(c.state == State::OEM_PASS, "no automotion after reboot");
    OK(c.sleepy["left"] == 40.0f, "settings retained"); }
  { Controller c = mk(); c.command("stop");
    OK(c.state == State::IDLE && c.queue.empty(), "stop->IDLE cancels"); }
  { Controller c = mk(); c.fault = "OVERTIME_TRIP"; c.state = State::FAULT;
    OK(!c.command("up:B"), "fault blocks motion");
    OK(c.command("clear") && c.state == State::IDLE, "clear->IDLE"); }
  { Controller c = mk();
    c.oem_press(); c.tick(0.3f); c.feed_watchdog(); c.oem_press(); run(c);
    OK(c.left.position == 100.0f && c.right.position == 0.0f, "oem 2-press wink:L"); }
  { Controller c = mk(); c.oem_press(); run(c, 3.0f);
    OK(c.left.position == 0.0f && c.right.position == 0.0f, "oem single = stock"); }
  { Controller c; c.boot(); c.feed_watchdog();
    OK(c.aux_press("aux1") == "dispatched", "aux momentary no-BLE");
    OK(c.aux_press("aux9") == "invalid-aux", "aux invalid rejected"); }
  { Controller c = mk();
    c.configure_aux("aux2", "blink:B", true);
    OK(c.aux_press("aux2") == "loop-started", "aux loop starts");
    OK(c.aux_press("aux2") == "loop-cancelled", "aux loop cancels"); }
  // D-02: diagnostic log bounded (parity with Python LOG_MAX).
  { Controller c = mk();
    for (int i = 0; i < 300; i++) { c.ble_connect(true); c.ble_disconnect(); }
    OK(c.log.size() == kLogMax, "log capped at kLogMax under abuse");
    OK(c.log.back() == "ble_disconnect->cancel_queue", "log keeps most-recent"); }
  // Pairing-PIN transport format (reconciliation 2026-09-23: firmware glue
  // must reject what app validatePasskey rejects — 6 ASCII digits only).
  // must reject what app validatePasskey rejects — 6 ASCII digits only).
  OK(isValidPairingPin("123456"), "pin 6-digit accepted");
  OK(!isValidPairingPin("12345"), "pin 5-digit rejected");
  OK(!isValidPairingPin("abcdef"), "pin non-digit rejected");
  OK(!isValidPairingPin("1234567"), "pin 7-char rejected");

  printf("PASS: %d core assertions (host logic only; GPIO/BLE/NVS UNVERIFIED)\n", n);
  return 0;
}
