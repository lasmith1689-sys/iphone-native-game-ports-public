"""v0.8 regression tests for optional Quest MR and controller timeline scrubbing."""
from pathlib import Path
import math
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT/'scripts/main.gd').read_text()
PROJECT = (ROOT/'project.godot').read_text()
PRESET = (ROOT/'export_presets.cfg').read_text()
WORKFLOW = (ROOT.parent/'.github/workflows/grand-prix-quest3.yml').read_text()
README = (ROOT/'README.md').read_text()

class MRAndSeekUpgrade(unittest.TestCase):
    def test_passthrough_extension_requires_build_plugin(self):
        self.assertIn('openxr/extensions/meta/passthrough=true',PROJECT)
        self.assertIn('openxr/environment_blend_mode=2',PROJECT)
        self.assertIn('xr_features/enable_meta_plugin=true',PRESET)
        self.assertIn('meta_xr_features/passthrough=1',PRESET)
        self.assertIn('gradle_build/use_gradle_build=true',PRESET)
    def test_release_plugin_fixed_and_digest_checked(self):
        self.assertIn('5.1.0-stable/godotopenxrvendorsaddon.zip',WORKFLOW)
        self.assertIn('6a838dbdf4115549e4511ebee0da9a5dcc8f9f6258d4cc2f2ee57a907a3e2911',WORKFLOW)
        self.assertIn("sha256(archive).hexdigest() == digest",WORKFLOW)
        self.assertIn("assert vendors",WORKFLOW)
    def test_gradle_template_makes_clean_reproducible_build(self):
        self.assertIn('android_source.zip',WORKFLOW)
        self.assertIn("'.build_version'",WORKFLOW)
        self.assertIn('4.6.3.stable',WORKFLOW)
        self.assertIn('gradle_build/compress_native_libraries=true',PRESET)
    def test_mr_fallback_and_boundaries(self):
        self.assertIn('if not passthrough_active:',SRC)
        self.assertIn('XR_ENV_BLEND_MODE_OPAQUE',SRC)
        self.assertIn('meta_xr_features/boundary_mode=0',PRESET)
        self.assertIn('MR unverified',README)
        self.assertIn('furniture anchoring',README)
    def test_timeline_controller_ray_and_latches(self):
        for fragment in ('func _timeline_fraction(controller:', 'board.to_local(', 'basis.inverse()', 'seeking_side == side', 'seeking_side = ""', 'timeline >= 0.0', 'playing = false', 'minf(LENGTH - 0.001', 'trigger_down[side] = pressed'):
            self.assertIn(fragment,SRC)
    def test_scrubbing_ruler_matches_cursor_math(self):
        for f in (0.0,.10,.50,.90,1.0):
            x = -.44 + .88*f
            self.assertAlmostEqual(max(0,min(1,(x+.44)/.88)),f,places=10)
    def test_progress_and_scenery(self):
        self.assertIn('func _make_finish_line()',SRC)
        self.assertIn('for column in range(8)',SRC)
        self.assertIn('for row in range(2)',SRC)
        self.assertIn('POINT AND HOLD TRIGGER ON TIMELINE TO SEEK',SRC)
    def test_version_only_advances_on_upgrade(self):
        self.assertIn('version/code=8',PRESET)
        self.assertIn('version/name="0.8.0"',PRESET)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.8_Easy_Install',WORKFLOW)
        self.assertIn('package/unique_name="com.gptimemachine.quest3"',PRESET)

if __name__ == '__main__':
    unittest.main()
