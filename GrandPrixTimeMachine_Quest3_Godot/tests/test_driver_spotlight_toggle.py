"""Source contracts for reversible driver inspection; not headset testing."""
from pathlib import Path
import unittest

SRC = (Path(__file__).resolve().parents[1] / "scripts/main.gd").read_text()


class DriverSpotlightToggle(unittest.TestCase):
    def test_same_driver_dismisses_spotlight(self):
        selection = SRC.split("func _select_driver(key: String)", 1)[1].split("func _update_spotlight()", 1)[0]
        self.assertIn('spotlight_driver = "" if spotlight_driver == key else key', selection)
        self.assertIn("if not DRIVER_CARDS.has(key):", selection)

    def test_dismissal_restores_default_card(self):
        update = SRC.split("func _update_spotlight()", 1)[1].split("func _update_story()", 1)[0]
        self.assertIn('if spotlight_driver == "":', update)
        self.assertIn('labels["focus_title"].text = "DRIVER SPOTLIGHT"', update)
        self.assertIn('labels["focus_facts"].text = "POINT AT ANY CAR + TRIGGER"', update)
        self.assertIn('labels["focus_note"].text = "SELECT AGAIN TO CLOSE | D CYCLES ON DESKTOP"', update)
        self.assertIn('visible = key == spotlight_driver', update)


if __name__ == "__main__":
    unittest.main()
