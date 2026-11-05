"""v0.8 regression tests for timeline scrubbing and MR-safe VR fallback."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT/'scripts/main.gd').read_text()
PROJECT = (ROOT/'project.godot').read_text()
PRESET = (ROOT/'export_presets.cfg').read_text()
WORKFLOW = (ROOT.parent/'.github'/'workflows'/'grand-prix-quest3.yml').read_text()
README = (ROOT/'README.md').read_text()
MR_NOTES = (ROOT/'docs/MR_CI_BLOCKER.md').read_text()

class TimelineAndReleaseSafety(unittest.TestCase):
    def test_mr_never_claimed_without_plugin(self):
        self.assertNotIn('openxr/extensions/meta/passthrough=true',PROJECT)
        self.assertNotIn('xr_features/enable_meta_plugin=true',PRESET)
        self.assertIn('not yet bundled',README)
        self.assertIn('MR unverified',README)
        self.assertIn('5.1.0',MR_NOTES)
        self.assertIn('5.0.1',MR_NOTES)
    def test_ci_retains_reproducible_direct_export(self):
        self.assertIn('gradle_build/use_gradle_build=false',PRESET)
        self.assertIn('python3 tools/verify_apk.py',WORKFLOW)
        self.assertIn('apksigner',WORKFLOW)
        self.assertIn('SHA256SUMS.txt',WORKFLOW)
        self.assertNotIn('Install official pinned OpenXR Vendors',WORKFLOW)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.12_Easy_Install',WORKFLOW)
    def test_fallback_does_not_fake_passthrough(self):
        self.assertIn('if not passthrough_active:',SRC)
        self.assertIn('XR_ENV_BLEND_MODE_OPAQUE',SRC)
        self.assertIn('get_viewport().transparent_bg = false',SRC)
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
    def test_version_and_stable_package_id(self):
        self.assertIn('version/code=12',PRESET)
        self.assertIn('version/name="0.12.0"',PRESET)
        self.assertIn('package/unique_name="com.gptimemachine.quest3"',PRESET)
if __name__ == '__main__':
    unittest.main()
