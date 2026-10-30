"""Source-contract regression tests for the standalone Quest v0.6 update."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT/'scripts/main.gd').read_text()
SCENE = (ROOT/'scenes/main.tscn').read_text()
PRESET = (ROOT/'export_presets.cfg').read_text()


class InputUpgradeContracts(unittest.TestCase):
    def test_both_aim_poses_and_beams(self):
        self.assertIn('tracker = "left_hand"\npose = "aim"', SCENE)
        self.assertIn('tracker = "right_hand"\npose = "aim"', SCENE)
        self.assertIn('_add_pointer(left_controller, "left")', SOURCE)
        self.assertIn('_add_pointer(right_controller, "right")', SOURCE)

    def test_loss_of_tracking_hides_pointer_and_resets_latches(self):
        self.assertIn('get_is_active() and controller.get_has_tracking_data()', SOURCE)
        self.assertIn('(beams[side] as MeshInstance3D).visible = tracked', SOURCE)
        self.assertIn('trigger_down[side] = false', SOURCE)
        self.assertIn('center_down[side] = false', SOURCE)

    def test_one_action_per_trigger_down(self):
        self.assertIn('if pressed and not bool(trigger_down[side]) and hit != "":', SOURCE)
        self.assertIn('trigger_down[side] = pressed', SOURCE)
        self.assertIn('func _show_hover(action: String)', SOURCE)

    def test_recenter_shortcut(self):
        self.assertIn('"CENTER", 0.50', SOURCE)
        self.assertIn('controller.is_button_pressed("by_button")', SOURCE)
        self.assertIn('if center_pressed and not bool(center_down[side]):', SOURCE)

    def test_accessible_label_orientation(self):
        self.assertIn('hint.rotation.x = 0.0', SOURCE)
        self.assertIn('label3d.rotation.x = 0.0', SOURCE)
        self.assertIn('AIM + TRIGGER TO SELECT', SOURCE)

    def test_safe_moderate_expanded_scale(self):
        self.assertIn('2.30 if expanded else tabletop_scale', SOURCE)
        self.assertIn('2.05 if expanded else 0.91', SOURCE)

    def test_small_scene_geometry(self):
        self.assertIn('cone.radial_segments = 6', SOURCE)
        self.assertIn('for i in range(7):', SOURCE)

    def test_android_version_changes_without_package_name(self):
        self.assertIn('version/code=6', PRESET)
        self.assertIn('version/name="0.6.0"', PRESET)
        self.assertIn('package/unique_name="com.gptimemachine.quest3"', PRESET)


if __name__ == '__main__':
    unittest.main(verbosity=2)
