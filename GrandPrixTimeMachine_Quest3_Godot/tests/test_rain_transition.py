"""Fuji rain transition source contracts; headset appearance is unverified."""
from pathlib import Path
import unittest

SRC = (Path(__file__).resolve().parents[1] / "scripts/main.gd").read_text()


class RainTransitionTests(unittest.TestCase):
    def test_gradual_drying_windows(self):
        self.assertIn("lerpf(1.0, 0.5, clampf((playback - 28.0) / 32.0", SRC)
        self.assertIn("lerpf(0.5, 0.21, clampf((playback - 60.0) / 16.0", SRC)
        self.assertIn("clampi(roundi(48.0 * rain_fraction), 1, 48)", SRC)

    def test_instancing_budget_preserved(self):
        self.assertIn("multi.instance_count = 48", SRC)
        self.assertIn("rain_mesh.visible = chapter < 5", SRC)
        self.assertIn("multi.set_instance_transform(i,", SRC)
        self.assertNotIn("GPUParticles3D.new()", SRC)


if __name__ == "__main__":
    unittest.main()
