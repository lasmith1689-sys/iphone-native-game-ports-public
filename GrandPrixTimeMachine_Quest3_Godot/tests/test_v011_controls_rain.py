"""v0.11 source contracts; no actual headset validation is inferred."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT/'scripts/main.gd').read_text()
PRESET = (ROOT/'export_presets.cfg').read_text()
WORKFLOW = (ROOT.parent/'.github/workflows/grand-prix-quest3.yml').read_text()

class ShortcutAndRainTests(unittest.TestCase):
    def test_both_controller_play_pause_latches(self):
        self.assertIn('controller.is_button_pressed("ax_button")', SRC)
        self.assertIn('quick_pressed and not bool(quick_pause_down[side])', SRC)
        self.assertIn('quick_pause_down[side] = quick_pressed', SRC)
        self.assertIn('quick_pause_down[side] = false', SRC)
    def test_stick_hysteresis_and_single_step(self):
        for s in ('var thumbstick: Vector2 = controller.get_vector2("primary")','axis > 0.75','axis < -0.75','absf(axis) < 0.35','stick_direction[side] = 0','elif step != 0 and int(stick_direction[side]) == 0:','_action("next" if step > 0 else "previous")'):
            self.assertIn(s,SRC)
    def test_one_action_per_frame_remains(self):
        self.assertGreaterEqual(SRC.count('action_taken = true'),6)
        self.assertIn('if not action_taken and seeking_side == "":',SRC)
        self.assertIn('seeking_side = ""',SRC)
    def test_rain_instancing_and_budget(self):
        for s in ('MultiMesh.new()','multi.instance_count = 48','rain_mesh = MultiMeshInstance3D.new()','multi.set_instance_transform(i,','if rain_accum >= 0.04:','rain_mesh.visible = chapter < 5'):
            self.assertIn(s,SRC)
        self.assertNotIn('GPUParticles3D.new()',SRC)
    def test_release_metadata(self):
        self.assertIn('version/code=14',PRESET)
        self.assertIn('version/name="0.14.0"',PRESET)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.14_Easy_Install',WORKFLOW)
    def test_no_mr_claim(self):
        readme=(ROOT/'README.md').read_text()
        self.assertIn('VR-first',readme)
        self.assertIn('MR unverified',readme)

if __name__ == '__main__':
    unittest.main()
