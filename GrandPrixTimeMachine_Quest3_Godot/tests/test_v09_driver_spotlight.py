"""Static regressions for Quest v0.9; do not imply physical headset testing."""
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT/"scripts/main.gd").read_text()
PRESET = (ROOT/"export_presets.cfg").read_text()
WORKFLOW = (ROOT.parent/".github/workflows/grand-prix-quest3.yml").read_text()
README = (ROOT/"README.md").read_text()
class DriverSpotlights(unittest.TestCase):
    def test_f1_results(self):
        for name, number, team, result in [("ANDRETTI","#5","LOTUS-FORD","73 LAPS"),("HUNT","#11","MCLAREN-FORD","72 LAPS"),("LAUDA","#1","FERRARI","WITHDREW AFTER 2 LAPS"),("DEPAILLER","#4","TYRRELL P34","72 LAPS")]:
            with self.subTest(name=name):
                self.assertIn('"'+name+'": {"heading": ',SRC)
                self.assertIn(number,SRC); self.assertIn(team,SRC); self.assertIn(result,SRC)
    def test_ray_and_trigger(self):
        for s in ('func _hit_driver(controller:', '_hit_driver(controller) if hit == ""','current_hover = "driver:" + driver','_select_driver(driver)','if not car.visible:'):
            self.assertIn(s,SRC)
    def test_priority_and_one_action(self):
        self.assertIn('if hit == "" and timeline < 0.0 else ""',SRC)
        self.assertIn('if not action_taken and seeking_side == "":',SRC)
        self.assertIn('trigger_down[side] = pressed',SRC)
    def test_beacon_and_stationary_viewer(self):
        for s in ('car.add_child(beacon)','beacon.visible = false','key == spotlight_driver and (car_nodes[key] as Node3D).visible','(car_nodes[spotlight_driver] as Node3D).visible'):
            self.assertIn(s,SRC)
        self.assertNotIn('cam.global_position = car',SRC)
        self.assertIn('The viewer never moves or chases a car',README)
    def test_desktop_access(self):
        self.assertIn('KEY_D:',SRC); self.assertIn('_select_driver(DRIVER_ORDER[next_driver])',SRC)
    def test_card_labels(self):
        for label in ('focus_title','focus_facts','focus_note'): self.assertIn('_label("'+label+'"',SRC)
        self.assertLess(SRC.index('_label("focus_title"'), SRC.index('func _add_button'))
    def test_release(self):
        self.assertIn('version/code=14',PRESET); self.assertIn('version/name="0.14.0"',PRESET)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.14_Easy_Install',WORKFLOW)
    def test_sources(self):
        ledger=(ROOT/"docs/HISTORICAL_SOURCES.md").read_text()
        self.assertIn('Driver spotlight: Andretti #5',ledger)
        self.assertIn('formula1.com/en/results/1976/races/373/japan/race-result',ledger)
if __name__ == "__main__": unittest.main()
