"""Quest source contract and independent numerical checks.

These tests deliberately do NOT claim Godot parsing, Android build or headset
validation. They catch regressions in the geometry, interaction setup, and
historical citation ledger that earlier plain-string tests missed.
"""
import math
from pathlib import Path
import re
import unittest

from test_quest import ROOT, raw

SOURCE = (ROOT / 'scripts' / 'main.gd').read_text()
TRACK = (ROOT / 'scripts' / 'track.gd').read_text()
CONFIG = (ROOT / 'export_presets.cfg').read_text()
PROJECT = (ROOT / 'project.godot').read_text()
SCENE = (ROOT / 'scenes' / 'main.tscn').read_text()
LEDGER = (ROOT / 'docs' / 'HISTORICAL_SOURCES.md').read_text()
README = (ROOT / 'README.md').read_text()
WORKFLOW = (ROOT.parent / '.github' / 'workflows' / 'grand-prix-quest3.yml')


def segment_crosses(a, b, c, d):
    """Interior intersection test for noncollinear 2-D line segments."""
    def cross(x, y, z):
        return (y[0]-x[0])*(z[1]-x[1])-(y[1]-x[1])*(z[0]-x[0])
    ab1, ab2 = cross(a, b, c), cross(a, b, d)
    cd1, cd2 = cross(c, d, a), cross(c, d, b)
    return ab1*ab2 < -1.e-12 and cd1*cd2 < -1.e-12


class SourceBehaviorContracts(unittest.TestCase):
    def test_historical_event_times_are_ordered_and_within_playback(self):
        times = [float(x) for x in re.findall(r'\{"at": ([0-9.]+), "title":', SOURCE)]
        self.assertEqual(len(times), 6)
        self.assertEqual(times[0], 0.0)
        self.assertEqual(times, sorted(times))
        playback_length = float(re.search(r'const LENGTH := ([0-9.]+)', SOURCE).group(1))
        self.assertLess(times[-1], playback_length)

    def test_buttons_have_distinct_actions(self):
        btns = re.findall(r'_add_button\("[^\"]+",\s*[-0-9.]+,\s*Color\([^\n]+?,\s*"(\w+)"\)', SOURCE)
        self.assertEqual(set(btns), {'pause', 'previous', 'next', 'scale', 'speed', 'center'})
        self.assertEqual(len(btns), len(set(btns)))

    def test_controls_stay_separate_from_scaling_diorama(self):
        self.assertIn('var ui_panel: Node3D', SOURCE)
        self.assertIn('var orb := _sphere(ui_panel,', SOURCE)
        self.assertNotIn('_sphere(board, "Action ', SOURCE)
        self.assertIn('ui_panel.global_position = cam.global_position + forward * 0.68', SOURCE)
        self.assertIn('ui_panel.rotation.y = board.rotation.y', SOURCE)
        self.assertIn('var radius: float = 0.066', SOURCE)
        self.assertNotIn('0.066 * board.scale.x', SOURCE)

    def test_camera_facing_yaw_sign(self):
        # GDScript yaw is atan2(-forward.x, -forward.z). The resulting
        # panel normal (+Z in local coordinates) must point toward the user.
        self.assertIn('board.rotation.y = atan2(-forward.x, -forward.z)', SOURCE)
        for degrees in range(0, 360, 15):
            a = math.radians(degrees)
            fx, fz = math.sin(a), math.cos(a)
            yaw = math.atan2(-fx, -fz)
            self.assertAlmostEqual(math.sin(yaw), -fx, places=10)
            self.assertAlmostEqual(math.cos(yaw), -fz, places=10)

    def test_recenter_handles_vertical_head_pose(self):
        self.assertIn('if forward.length_squared() < 0.0001:', SOURCE)
        self.assertIn('forward = Vector3.FORWARD', SOURCE)

    def test_controller_action_path(self):
        self.assertIn('controller.global_transform.basis.z.normalized()', SOURCE)
        self.assertIn('get_float("trigger")', SOURCE)
        self.assertIn('is_button_pressed("trigger_click")', SOURCE)
        self.assertIn('if pressed and not bool(trigger_down[side]) and hit != "":', SOURCE)
        self.assertIn('if along < 0.025 or along > 2.0:', SOURCE)

    def test_xr_nodes_match_controller_path(self):
        self.assertIn('[node name="RightController" type="XRController3D" parent="XROrigin3D"]', SCENE)
        self.assertIn('tracker = "right_hand"', SCENE)
        self.assertIn('tracker = "left_hand"', SCENE)
        self.assertIn('[node name="XRCamera3D" type="XRCamera3D" parent="XROrigin3D"]', SCENE)

    def test_mr_has_transparent_alpha_environment(self):
        self.assertIn('environment.background_mode = Environment.BG_COLOR', SOURCE)
        self.assertIn('environment.background_color = Color(0.0, 0.0, 0.0, 0.0)', SOURCE)
        self.assertIn('get_viewport().transparent_bg = true', SOURCE)

    def test_fallback_is_not_pretended_mr(self):
        self.assertIn('if not passthrough_active:', SOURCE)
        self.assertIn('XRInterface.XR_ENV_BLEND_MODE_OPAQUE', SOURCE)
        self.assertIn('get_viewport().transparent_bg = false', SOURCE)
        self.assertIn('not yet bundled', README)
        self.assertIn('MR unverified', README)

    def test_no_hand_tracking_claim(self):
        self.assertIn('Hand tracking and room surface anchoring are **not implemented**', README)

    def test_scene_bootstrap_is_deferred(self):
        self.assertIn('call_deferred("_start_xr")', SOURCE)
        self.assertIn('get_viewport().use_xr = true', SOURCE)
        self.assertIn('desktop_camera.current = true', SOURCE)

    def test_software_render_compatible_with_standalone(self):
        self.assertIn('renderer/rendering_method="gl_compatibility"', PROJECT)
        self.assertIn('openxr/enabled=true', PROJECT)

    def test_android_debug_package_has_64bit_only(self):
        self.assertIn('platform="Android"', CONFIG)
        self.assertIn('architectures/arm64-v8a=true', CONFIG)
        self.assertIn('architectures/armeabi-v7a=false', CONFIG)
        self.assertIn('xr_features/xr_mode=1', CONFIG)

    def test_no_nonfree_assets_embedded(self):
        self.assertFalse(list(ROOT.rglob('*.mp4')))
        self.assertFalse(list(ROOT.rglob('*.wav')))
        self.assertFalse(list(ROOT.rglob('*.png')))

    def test_standalone_ci_has_quiet_source_gate(self):
        if not WORKFLOW.exists():
            self.skipTest('CI workflow only available inside the repository checkout')
        c = WORKFLOW.read_text()
        self.assertIn('barichello/godot-ci:4.6.3', c)
        self.assertIn('--headless --editor --path .', c)
        self.assertIn('--export-debug "Quest3"', c)
        self.assertIn('python3 tools/verify_apk.py', c)
        self.assertIn('SHA256SUMS.txt', c)

    def test_official_race_result_link_is_correct_year(self):
        self.assertIn('https://www.formula1.com/en/results/1976/races/373/japan/race-result', LEDGER)
        self.assertNotIn('/1977/races/373/south-africa/', LEDGER)

    def test_historical_disclaimer_stays_in_world(self):
        self.assertIn('ILLUSTRATIVE MODEL | NOT LAP ACCURATE', SOURCE)
        self.assertIn('illustrative', README.lower())

    def test_manual_installation_requires_developer_mode(self):
        self.assertIn('Developer Mode must be enabled', README)
        self.assertIn('adb install -r', README)


class IndependentTrackGeometry(unittest.TestCase):
    def setUp(self):
        self.pts = [raw(i/320.0) for i in range(320)]

    def test_closed_track(self):
        self.assertLess(math.dist(raw(1.0), self.pts[0]), 1.0e-10)

    def test_no_centerline_self_intersections(self):
        pts = self.pts
        n = len(pts)
        for i in range(n):
            a, b = pts[i], pts[(i+1)%n]
            for j in range(i+2, n):
                if i == 0 and j == n-1:
                    continue
                c, d = pts[j], pts[(j+1)%n]
                self.assertFalse(segment_crosses(a, b, c, d), f'crossing segments {i}, {j}')

    def test_track_inside_base(self):
        for x, z in self.pts:
            self.assertLess(abs(x)+0.05, 1.29/2)
            self.assertLess(abs(z-0.045)+0.05, 0.78/2)

    def test_sampling_has_no_zero_length_segment(self):
        lengths = [math.dist(self.pts[i], self.pts[(i+1)%len(self.pts)]) for i in range(len(self.pts))]
        self.assertGreater(min(lengths), 1.e-4)
        self.assertLess(max(lengths), 0.03)

    def test_track_shape_is_period_illustration(self):
        self.assertIn('Fuji-inspired, NOT a surveyed version', TRACK)
        self.assertIn('not the period Fuji layout', ' '.join(LEDGER.replace('*', '').split()))


if __name__ == '__main__':
    unittest.main(verbosity=2)
