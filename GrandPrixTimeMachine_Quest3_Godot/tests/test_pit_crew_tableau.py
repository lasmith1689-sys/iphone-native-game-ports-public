"""Source contracts for illustrative pit-stop staging, not headset rendering."""
from pathlib import Path
import unittest

SRC = (Path(__file__).resolve().parents[1] / "scripts/main.gd").read_text()


class PitCrewTableau(unittest.TestCase):
    def test_crew_only_appears_during_pit_stop(self):
        build = SRC.split("func _build_diorama()", 1)[1].split("func _add_button(", 1)[0]
        update = SRC.split("func _update_cars()", 1)[1].split("func _advance_playback(", 1)[0]
        self.assertIn('pit_crew.visible = false', build)
        self.assertIn('playback >= 90.0 and playback < 103.0', update)
        self.assertIn('pit_crew.visible = hunt_in_pits', update)

    def test_crew_is_small_and_asset_free(self):
        build = SRC.split("func _build_diorama()", 1)[1].split("func _add_button(", 1)[0]
        self.assertIn('for i in range(3):\n        var x := -0.245', build)
        self.assertIn('_box(pit_crew, "crew torso"', build)
        self.assertIn('_sphere(pit_crew, "crew helmet"', build)
        self.assertIn('_box(pit_crew, "crew legs"', build)


if __name__ == "__main__":
    unittest.main()
