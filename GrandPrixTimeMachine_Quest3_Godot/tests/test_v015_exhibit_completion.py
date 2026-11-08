""""v0.15 one-shot exhibit playback and intentional replay contracts.

These source-contract tests do not simulate OpenXR or a physical Quest 3.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "scripts/main.gd").read_text()
README = (ROOT / "README.md").read_text()
PRESET = (ROOT / "export_presets.cfg").read_text()
WORKFLOW = (ROOT.parent / ".github/workflows/grand-prix-quest3.yml").read_text()


def advance(position, delta, speed, length=138.0):
    """Independent model of the monotonic, clamped exhibit clock."""
    next_position = min(length, position + max(0.0, delta) * speed)
    return next_position, next_position >= length


def tick(position, delta, speed, playing, length=138.0):
    """Model the _process gate: a stopped exhibit does not advance again."""
    if not playing:
        return position, False
    next_position, completed = advance(position, delta, speed, length)
    return next_position, not completed


class ExhibitCompletion(unittest.TestCase):
    def test_process_uses_dedicated_advance_helper(self):
        process = SRC.split("func _process(delta: float)", 1)[1].split("func _read_controller()", 1)[0]
        self.assertIn("_advance_playback(delta)", process)
        self.assertNotIn("fposmod(playback", process)

    def test_clock_is_monotonic_and_clamped(self):
        block = SRC.split("func _advance_playback(delta: float)", 1)[1].split("func _process(delta: float)", 1)[0]
        self.assertIn("minf(playback + maxf(delta, 0.0) * speed, LENGTH)", block)
        self.assertNotIn("fposmod(", block)
        self.assertIn("if playback >= LENGTH:", block)
        self.assertIn("playback = LENGTH", block)
        self.assertIn("playing = false", block)

    def test_final_chapter_is_selected_before_completion(self):
        block = SRC.split("func _advance_playback(delta: float)", 1)[1].split("func _process(delta: float)", 1)[0]
        self.assertLess(block.index("if previous != new_chapter:"), block.index("if playback >= LENGTH:"))
        self.assertIn("for i in range(1, BEATS.size()):", block)
        self.assertIn('"title": "WORLD CHAMPION"', SRC)

    def test_completion_does_not_repeatedly_advance(self):
        # The guard belongs in _process, not redundantly inside the helper.
        # This keeps every call site subject to the same playing state.
        process = SRC.split("func _process(delta: float)", 1)[1].split("func _read_controller()", 1)[0]
        self.assertIn("if playing:\\n        _advance_playback(delta)", process)
        block = SRC.split("func _advance_playback(delta: float)", 1)[1].split("func _process(delta: float)", 1)[0]
        self.assertIn("playing = false", block)
        self.assertIn('print("GPTM exhibit completed; playback stopped at final chapter")', block)
        position, playing = tick(137.9, 1.0, 1.0, True)
        self.assertEqual(position, 138.0)
        self.assertFalse(playing)
        self.assertEqual(tick(position, 1.0, 4.0, playing), (138.0, False))

    def test_play_restarts_only_after_natural_completion(self):
        action = SRC.split("func _action(action: String)", 1)[1].split("func _recenter()", 1)[0]
        pause = action.split('"pause":', 1)[1].split('"next":', 1)[0]
        self.assertIn("if not playing and playback >= LENGTH:", pause)
        self.assertIn("playback = 0.0", pause)
        self.assertIn("chapter = 0", pause)
        self.assertIn("_update_story()", pause)
        self.assertIn("playing = not playing", pause)

    def test_end_state_is_visible_and_replay_labelled(self):
        self.assertIn('labels["state"].text = "REPLAY READY" if not playing and playback >= LENGTH', SRC)
        self.assertIn("REPLAY READY", README)
        self.assertIn("Press PLAY or A/X", README)

    def test_manual_pause_and_speed_remain_supported(self):
        self.assertIn('"speed": speed = 1.0 if speed >= 4.0 else speed * 2.0', SRC)
        for rate in (1.0, 2.0, 4.0):
            pos, ended = advance(137.0, 1.0, rate)
            self.assertEqual(pos, 138.0)
            self.assertTrue(ended)

    def test_large_delta_and_negative_delta_are_safe(self):
        self.assertEqual(advance(137.9, 5.0, 4.0), (138.0, True))
        self.assertEqual(advance(50.0, -2.0, 1.0), (50.0, False))
        self.assertEqual(advance(0.0, 0.0, 1.0), (0.0, False))

    def test_release_metadata_is_consistent(self):
        self.assertIn("version/code=15", PRESET)
        self.assertIn('version/name="0.15.0"', PRESET)
        self.assertIn("GrandPrixTimeMachine_Quest3_v0.15_Easy_Install", WORKFLOW)
        self.assertIn("v0.15", README)


if __name__ == "__main__":
    unittest.main(verbosity=2)
