"""Guard XR focus and controller reacquisition safety at the source-contract level.

These tests cannot substitute for physical Quest 3 verification.
"""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "scripts/main.gd").read_text()


def function_body(name):
    marker = f"func {name}("
    start = SOURCE.index(marker)
    end = SOURCE.find("\nfunc ", start + len(marker))
    return SOURCE[start:end if end >= 0 else len(SOURCE)]


class FocusSafetyContracts(unittest.TestCase):
    def test_lost_focus_pauses_before_clearing_inputs(self):
        body = function_body("_on_openxr_visible")
        self.assertLess(body.index("restore_playback_on_focus = playing"),
                        body.index("playing = false"))
        self.assertLess(body.index("playing = false"),
                        body.index("_clear_xr_input_latches()"))

    def test_restore_does_not_unpause_manual_pause(self):
        body = function_body("_on_openxr_focussed")
        self.assertIn("if restore_playback_on_focus:", body)
        self.assertIn("playing = true", body)
        self.assertIn("restore_playback_on_focus = false", body)
        self.assertIn("_clear_xr_input_latches()", body)

    def test_tracking_reacquisition_does_not_fire_held_inputs(self):
        body = function_body("_read_controller")
        guard = body.split('if not bool(controller_was_tracked[side]):', 1)[1]
        guard = guard.split('var hit := _hit_control(controller)', 1)[0]
        self.assertIn("continue", guard)
        for latch in ("trigger_down[side]", "center_down[side]",
                      "quick_pause_down[side]", "stick_direction[side]",
                      "stick_vertical[side]"):
            self.assertIn(latch, guard)

    def test_xr_pointer_is_hidden_when_inputs_cleared(self):
        body = function_body("_clear_xr_input_latches")
        self.assertIn("controller_was_tracked[side] = false", body)
        self.assertIn('(beams[side] as MeshInstance3D).visible = false', body)

    def test_frame_metrics_are_not_misrepresented_as_compositor_fps(self):
        body = function_body("_sample_frame_timing")
        self.assertIn("if not xr_active or not xr_focused", body)
        self.assertIn("app_fps=", body)
        self.assertIn("cannot prove compositor FPS", body)


if __name__ == "__main__":
    unittest.main()
