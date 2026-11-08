"""Source contracts for v0.14 OpenXR frame pacing, focus and telemetry.
These DO NOT constitute tests of physical Quest hardware.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "scripts/main.gd").read_text()
README = (ROOT / "README.md").read_text()
PRESET = (ROOT / "export_presets.cfg").read_text()
WORKFLOW = (ROOT.parent / ".github/workflows/grand-prix-quest3.yml").read_text()


def sampled_fps(intervals):
    """Independent average app frame-interval estimate."""
    elapsed = sum(max(0.0, dt) for dt in intervals)
    return round(len(intervals) / elapsed) if elapsed > 0.0 else -1


class FramePacingAndFocus(unittest.TestCase):
    def test_vsync_only_after_xr_success(self):
        self.assertLess(SRC.index('if xr_interface != null and xr_interface.initialize():'),
                        SRC.index("DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)"))

    def test_signals_registered_before_initialize(self):
        start = SRC.index('if xr_interface != null and xr_interface.initialize():')
        for signal in ('session_begun', 'session_visible', 'session_focussed'):
            self.assertLess(SRC.index('xr_interface.connect("' + signal + '"'), start)

    def test_session_rate_guarded_and_not_forced(self):
        self.assertIn('xr_interface.call("get_display_refresh_rate")', SRC)
        self.assertIn("if reported_hz >= 60.0 and reported_hz <= 144.0:", SRC)
        self.assertIn("Engine.physics_ticks_per_second = roundi(reported_hz)", SRC)
        self.assertNotIn("set_display_refresh_rate(", SRC)

    def test_focus_loss_and_controller_latches(self):
        lost = SRC.split("func _on_openxr_visible()", 1)[1].split("func _on_openxr_focussed()", 1)[0]
        self.assertIn("restore_playback_on_focus = playing", lost)
        self.assertIn("playing = false", lost)
        self.assertIn("_clear_xr_input_latches()", lost)
        for v in ("trigger_down", "center_down", "quick_pause_down", "stick_direction", "stick_vertical"):
            self.assertIn(v + "[side] = 0" if v.startswith("stick_") else v + "[side] = false", SRC)
        self.assertIn('seeking_side = ""', SRC)
        self.assertIn("if not xr_active or not xr_focused:", SRC)

    def test_restores_only_prior_playback(self):
        focus = SRC.split("func _on_openxr_focussed()", 1)[1].split("func _sample_frame_timing()", 1)[0] if "func _sample_frame_timing()" in SRC else SRC.split("func _on_openxr_focussed()", 1)[1].split("func _sample_frame_timing(delta", 1)[0]
        self.assertIn("if restore_playback_on_focus:", focus)
        self.assertIn("playing = true", focus)
        self.assertIn("restore_playback_on_focus = false", focus)

    def test_warmup_cadence_and_telemetry(self):
        self.assertIn("intro_time < 3.0", SRC)
        self.assertIn("frame_window_seconds >= 2.0", SRC)
        self.assertIn("GPTM PERF app_fps=", SRC)
        self.assertIn("max_frame_ms=", SRC)
        self.assertIn("| FPS: %s", SRC)
        self.assertEqual(sampled_fps([1/72] * 144), 72)
        self.assertEqual(sampled_fps([1/90] * 180), 90)

    def test_version_and_explanatory_limits(self):
        self.assertIn("version/code=15", PRESET)
        self.assertIn('version/name="0.15.0"', PRESET)
        self.assertIn("GrandPrixTimeMachine_Quest3_v0.15_Easy_Install", WORKFLOW)
        self.assertIn("not** compositor FPS", README)
        self.assertIn("not** been tested", README)


if __name__ == "__main__":
    unittest.main()
