"""Controller reacquisition safety regression contracts.

Source-only tests: they do not simulate actual Quest tracking or inputs.
"""
from pathlib import Path
import unittest

SRC = (Path(__file__).resolve().parents[1] / "scripts/main.gd").read_text()


class ReacquisitionSafety(unittest.TestCase):
    def test_tracking_history_is_independent_per_hand(self):
        self.assertIn('var controller_was_tracked := {"left": false, "right": false}', SRC)
        self.assertIn('for side in ["left", "right"]:', SRC)

    def test_loss_of_tracking_clears_history(self):
        lost = SRC.split("if not tracked:", 1)[1].split("continue", 1)[0]
        self.assertIn("controller_was_tracked[side] = false", lost)
        self.assertIn('if seeking_side == side:', lost)
        self.assertIn('seeking_side = ""', lost)

    def test_reacquisition_consumes_held_inputs_before_raycast(self):
        block = SRC.split('if not bool(controller_was_tracked[side]):', 1)[1].split('var hit := _hit_control(controller)', 1)[0]
        self.assertIn('controller_was_tracked[side] = true', block)
        self.assertIn('trigger_down[side] = controller.get_float("trigger")', block)
        self.assertIn('center_down[side] = controller.is_button_pressed("by_button")', block)
        self.assertIn('quick_pause_down[side] = controller.is_button_pressed("ax_button")', block)
        self.assertIn('stick_direction[side] =', block)
        self.assertIn('stick_vertical[side] =', block)
        self.assertIn('continue', block)
        self.assertNotIn('_action(', block)

    def test_focus_reset_rearms_tracking_guard(self):
        block = SRC.split('func _clear_xr_input_latches()', 1)[1].split('func _on_openxr_visible()', 1)[0]
        self.assertIn('controller_was_tracked[side] = false', block)

    def test_release_then_press_still_works(self):
        read = SRC.split('func _read_controller()', 1)[1].split('func _action(', 1)[0]
        self.assertIn('if pressed and not bool(trigger_down[side])', read)
        self.assertIn('trigger_down[side] = pressed', read)
        self.assertIn('if absf(axis) < 0.35:', read)
        self.assertIn('if absf(vertical_axis) < 0.35:', read)


if __name__ == "__main__":
    unittest.main()
