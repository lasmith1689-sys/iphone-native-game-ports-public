"""Guard the lightweight, original 1970s car silhouette details."""
from pathlib import Path
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "scripts" / "main.gd").read_text(encoding="utf-8")


class CarVisualDetailContract(unittest.TestCase):
    def test_cockpit_and_sidepods_are_present(self):
        for name in ("cockpit surround", "cockpit opening", "left sidepod",
                     "right sidepod", "roll hoop", "rear diffuser silhouette"):
            with self.subTest(part=name):
                self.assertIn('"' + name + '"', SOURCE)

    def test_both_wings_have_endplates(self):
        for wing in ("front", "rear"):
            for side in ("left", "right"):
                with self.subTest(wing=wing, side=side):
                    self.assertIn('"' + wing + ' wing ' + side + ' endplate"', SOURCE)

    def test_visual_detail_remains_primitive_based(self):
        # New details should remain built-in geometry, without runtime asset fetching.
        segment = SOURCE.split('    # Low-poly 1970s open-cockpit details.', 1)[1].split(
            '    # Tyrrell P34:', 1)[0]
        self.assertIn('_box(car,', segment)
        self.assertNotIn('load("http', segment)
        self.assertNotIn('preload("http', segment)


if __name__ == "__main__":
    unittest.main()
