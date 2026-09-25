"""Host tests: state machine, bounds, BLE auth, loss, watchdog, resets,
concurrency, OEM restoration, settings retention, OEM multi-press, aux."""
import sys, os, unittest
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "model"))
from headlight_controller import Controller, State

def mk(bonded=True, link=True):
    c = Controller(); c.boot(); c.pair(True) if bonded else None
    c.ble_connect(link); c.feed_watchdog()
    return c

def run(c, s=8.0, dt=0.05):
    t = 0.0
    while t < s:
        c.feed_watchdog(); c.tick(dt); t += dt

class TestFSM(unittest.TestCase):
    def test_boot_safe_no_motion(self):
        c = Controller(); c.boot()
        self.assertEqual(c.state, State.OEM_PASS)
        self.assertTrue(c.oem_path_ok)
        self.assertEqual((c.left.position, c.right.position), (0.0, 0.0))

    def test_up_down(self):
        c = mk(); c.command("up:B"); run(c)
        self.assertEqual((c.left.position, c.right.position), (100.0, 100.0))
        c.command("down:L"); run(c); self.assertEqual(c.left.position, 0.0)
        self.assertEqual(c.right.position, 100.0)

    def test_sleepy_bounds(self):
        c = mk()
        self.assertTrue(c.set_sleepy("left", 50))
        self.assertFalse(c.set_sleepy("left", 101))
        self.assertFalse(c.set_sleepy("right", -1))
        self.assertEqual(c.sleepy["left"], 50.0)

    def test_invalid_rejected(self):
        c = mk()
        self.assertFalse(c.command("fly:B"))
        self.assertEqual(c._queue, [])  # no motion queued
        self.assertIsNone(c._active["side"])
        run(c, 1.0); self.assertEqual(c.left.position, 0.0)

    def test_invalid_streak_triggers_lockout(self):
        c = mk()
        for _ in range(5):
            self.assertFalse(c.command("fly:B"))
            c.tick(0.01); c.feed_watchdog()
        # 5 consecutive invalid commands -> 5 s lockout: valid command refused
        self.assertFalse(c.command("up:B"))
        run(c, 6.0)  # lockout expires; watchdog fed by run()
        self.assertTrue(c.command("up:B"))

    def test_rate_budget_is_ten_per_second(self):
        c = mk()
        for _ in range(10):
            self.assertTrue(c.command("blink:B"))
            c.tick(0.01); c.feed_watchdog()
        # 11th command inside the 1 s window trips the lockout
        self.assertFalse(c.command("blink:B"))

    def test_wave_direction_rl_right_first(self):
        c = mk(); c.command("wave:RL")
        self.assertEqual([q["side"] for q in c._queue], ["right", "left"])
        run(c)
        self.assertEqual((c.left.position, c.right.position), (100.0, 100.0))

    def test_unauthenticated_rejected(self):
        c = Controller(); c.boot(); c.ble_connect(False)  # unbonded link
        c.feed_watchdog()
        self.assertFalse(c.command("up:B", authenticated=True))
        run(c, 1.0); self.assertEqual(c.left.position, 0.0)

    def test_rate_limit_lockout(self):
        c = mk()
        for _ in range(12):
            c.command("blink:B"); c.tick(0.01); c.feed_watchdog()
        # next command inside lockout should fail
        self.assertFalse(c.command("up:B"))

    def test_ble_loss_cancels_and_oem_live(self):
        c = mk(); c.command("wave:B"); c.tick(0.2)
        c.ble_disconnect()
        self.assertEqual(c._queue, [])
        self.assertTrue(c.oem_path_ok)

    def test_watchdog_resets_to_oem(self):
        c = mk(); c.command("up:B")
        c.tick(3.0)  # no feed -> WDT
        self.assertEqual(c.state, State.OEM_PASS)
        self.assertTrue(c.oem_path_ok)

    def test_reset_restores_oem(self):
        c = mk(); c.command("up:B"); run(c, 1.0)
        c.power_loss()
        self.assertEqual(c.state, State.OEM_PASS)
        self.assertTrue(c.oem_path_ok)

    def test_concurrent_staggered(self):
        c = mk(); c.command("up:B"); run(c)
        # both reached 100 but via staggered queue (queue drained in order)
        self.assertEqual((c.left.position, c.right.position), (100.0, 100.0))
        self.assertEqual(c.state, State.IDLE)
        self.assertEqual(c.fault, "")
        self.assertEqual(c._queue, [])

    def test_settings_retained_no_automotion_after_reboot(self):
        c = mk()
        self.assertTrue(c.set_sleepy("left", 40))
        self.assertTrue(c.set_sleepy("right", 60))
        c.power_loss()  # settings retained in NVS intent; no motion on boot
        run(c, 2.0)
        self.assertEqual(c.state, State.OEM_PASS)
        self.assertEqual((c.left.position, c.right.position), (0.0, 0.0))
        self.assertEqual((c.sleepy["left"], c.sleepy["right"]), (40.0, 60.0))

    def test_oem_press_two_winks_left(self):
        c = mk()
        c.oem_press(); c.tick(0.3); c.feed_watchdog()
        c.oem_press(); run(c, 8.0)
        # wink L from down: down then up
        self.assertEqual(c.left.position, 100.0)
        self.assertEqual(c.right.position, 0.0)

    def test_oem_single_press_is_stock(self):
        c = mk()
        c.oem_press(); run(c, 3.0)
        self.assertEqual((c.left.position, c.right.position), (0.0, 0.0))
        self.assertEqual(c._queue, [])

    def test_oem_overflow_resets(self):
        c = mk()
        for _ in range(10):
            c.oem_press(); c.tick(0.1); c.feed_watchdog()
        run(c, 3.0)
        self.assertEqual((c.left.position, c.right.position), (0.0, 0.0))

    def test_oem_ignored_with_lamps_on_by_default(self):
        c = mk(); c.headlamps_on = True
        c.oem_press(); c.oem_press(); run(c, 3.0)
        self.assertEqual((c.left.position, c.right.position), (0.0, 0.0))

    def test_oem_mapping_configurable(self):
        c = mk()
        self.assertTrue(c.map_button(2, "up:B"))
        self.assertFalse(c.map_button(1, "up:B"))
        self.assertFalse(c.map_button(10, "up:B"))
        self.assertTrue(c.configure_press_gap(500))
        self.assertFalse(c.configure_press_gap(100))
        c.oem_press(); c.oem_press(); run(c, 8.0)
        self.assertEqual((c.left.position, c.right.position), (100.0, 100.0))

    def test_aux_momentary_without_ble(self):
        c = Controller(); c.boot(); c.feed_watchdog()  # no BLE at all
        self.assertEqual(c.aux_press("aux1"), "dispatched")
        run(c, 8.0)
        self.assertEqual((c.left.position, c.right.position), (100.0, 100.0))

    def test_aux_invalid_rejected(self):
        c = mk()
        self.assertEqual(c.aux_press("aux9"), "invalid-aux")
        self.assertFalse(c.configure_aux("aux9", "up:B", False))
        self.assertFalse(c.configure_aux("aux1", "fly:B", False))

    def test_aux_loop_repeats_then_cancels(self):
        c = mk()
        self.assertTrue(c.configure_aux("aux2", "blink:B", True))
        self.assertEqual(c.aux_press("aux2"), "loop-started")
        run(c, 4.0)
        self.assertEqual(c._loop["aux"], "aux2")  # still looping
        self.assertEqual(c.aux_press("aux2"), "loop-cancelled")
        self.assertEqual(c._loop["aux"], None)
        self.assertEqual(c.state, State.IDLE)

    def test_aux_loop_cancelled_on_ble_loss(self):
        c = mk()
        c.configure_aux("aux2", "blink:B", True)
        c.aux_press("aux2"); c.tick(0.5); c.feed_watchdog()
        c.ble_disconnect()
        self.assertEqual(c._loop["aux"], None)
        self.assertEqual(c._queue, [])

    def test_oem_restoration_after_fault_clear(self):
        c = mk()
        c.fault = "OVERTIME_TRIP"; c.state = State.FAULT
        self.assertFalse(c.command("up:B"))
        self.assertTrue(c.command("clear"))
        self.assertEqual(c.state, State.IDLE)

    def test_log_bounded_128_under_abuse(self):
        # D-02: sustained control-path activity must not grow the log without
        # bound (target heap). Recent entries must be preserved.
        from headlight_controller import LOG_MAX
        c = mk()
        for _ in range(300):
            c.ble_connect(True); c.ble_disconnect()
        self.assertLessEqual(len(c.log), LOG_MAX)
        self.assertEqual(len(c.log), LOG_MAX)
        self.assertIn("ble_disconnect->cancel_queue", c.log[-1])

if __name__ == "__main__":
    unittest.main()
