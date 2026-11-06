"""Regression checks for Quest 3 v0.12 height controls and XR diagnostics.

These tests check code contracts and independently exercise the stick decision
thresholds; they do not represent a physical headset test.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "scripts" / "main.gd").read_text()
PRESET = (ROOT / "export_presets.cfg").read_text()
README = (ROOT / "README.md").read_text()


def chosen_axis(x, y):
    """Independent mirror of documented dominant-axis hysteresis behavior."""
    if abs(x) > .75 and abs(x) > abs(y) + .10:
        return "chapter"
    if abs(y) > .75 and abs(y) > abs(x) + .10:
        return "height"
    return "none"


class HeightAndDiagnostics(unittest.TestCase):
    def test_version_and_package_identity(self):
        self.assertIn('version/code=13', PRESET)
        self.assertIn('version/name="0.13.0"', PRESET)
        self.assertIn('package/unique_name="com.gptimemachine.quest3"', PRESET)

    def test_independent_vertical_latches(self):
        self.assertIn('var stick_vertical := {"left": 0, "right": 0}', SRC)
        self.assertIn('stick_vertical[side] = 0', SRC)
        self.assertIn('int(stick_vertical[side]) == 0', SRC)
        self.assertIn('stick_vertical[side] = vertical_step', SRC)
        self.assertIn('var stick_direction := {"left": 0, "right": 0}', SRC)

    def test_tracking_loss_clears_vertical_latch(self):
        block = SRC.split('if not tracked:', 1)[1].split('continue', 1)[0]
        self.assertIn('stick_vertical[side] = 0', block)
        self.assertIn('stick_direction[side] = 0', block)

    def test_safely_bounded_seated_height(self):
        self.assertIn('height_adjustment + 0.08 * vertical_step, -0.32, 0.32', SRC)
        self.assertIn('Vector3.UP * height_adjustment', SRC)
        self.assertIn('height_adjustment := 0.0', SRC)
        level = 0.0
        for _ in range(20):
            level = max(-0.32, min(0.32, level + 0.08))
        self.assertAlmostEqual(level, 0.32)
        for _ in range(20):
            level = max(-0.32, min(0.32, level - 0.08))
        self.assertAlmostEqual(level, -0.32)

    def test_horizontal_flick_not_diagonal_false_positive(self):
        self.assertIn('absf(axis) > absf(vertical_axis) + 0.10', SRC)
        self.assertEqual(chosen_axis(.9, .1), 'chapter')
        self.assertEqual(chosen_axis(.9, .88), 'none')
        self.assertEqual(chosen_axis(-.9, .3), 'chapter')

    def test_vertical_flick_not_diagonal_false_positive(self):
        self.assertIn('absf(vertical_axis) > absf(axis) + 0.10', SRC)
        self.assertEqual(chosen_axis(.1, .95), 'height')
        self.assertEqual(chosen_axis(.83, .87), 'none')
        self.assertEqual(chosen_axis(-.2, -.9), 'height')

    def test_xr_diagnostics_are_headset_visible_and_rate_limited(self):
        self.assertIn('_label("xr_status",', SRC)
        self.assertIn('diagnostics_clock >= 0.25', SRC)
        self.assertIn('get_has_tracking_data()', SRC)
        self.assertIn('MODE: %s | L: %s | R: %s | HEIGHT: %d CM', SRC)
        self.assertIn('MR (UNVERIFIED)', SRC)

    def test_no_user_movement_required(self):
        self.assertIn('board.global_position = cam.global_position', SRC)
        self.assertNotIn('xr_camera.position +=', SRC)
        self.assertNotIn('xr_camera.global_position +=', SRC)
        self.assertIn('seeking_side == ""', SRC)

    def test_desktop_preview_height_controls(self):
        self.assertIn('KEY_UP:', SRC)
        self.assertIn('KEY_DOWN:', SRC)
        self.assertIn('Up/Down arrows', README)

    def test_readme_does_not_claim_headset_validation(self):
        self.assertIn('not evidence that Quest 3 hardware has been tested', README)
        self.assertIn('not bundled or certified', README)


if __name__ == '__main__':
    unittest.main()
