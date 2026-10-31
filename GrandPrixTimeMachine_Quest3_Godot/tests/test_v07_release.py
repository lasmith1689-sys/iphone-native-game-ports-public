"""Regressions for historically sourced v0.7 visuals and consent-based updates."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/'scripts/main.gd').read_text()
INSTALL=(ROOT/'install/INSTALL_ON_QUEST3.ps1').read_text()
LEDGER=(ROOT/'docs/HISTORICAL_SOURCES.md').read_text()
PRESET=(ROOT/'export_presets.cfg').read_text()
WORKFLOW=(ROOT.parent/'.github/workflows/grand-prix-quest3.yml').read_text()

class Version07Tests(unittest.TestCase):
    def test_official_lap_distance_in_display(self):
        self.assertIn('73 LAPS  |  4.359 KM  |  FUJI', SRC)
        self.assertIn('73 laps on the 4.359 km Fuji circuit', LEDGER)
    def test_entry_numbers_correct(self):
        for entry in ('"ANDRETTI": "5"', '"DEPAILLER": "4"', '"HUNT": "11"', '"LAUDA": "1"'):
            self.assertIn(entry, SRC)
    def test_six_wheeler_front_axles(self):
        for entry in ('if key == "DEPAILLER":','Vector2(-0.028, -0.041)','Vector2(-0.028, -0.018)','Vector2(-0.030, 0.030)','front_radius := 0.007'):
            self.assertIn(entry, SRC)
        self.assertIn('Tyrrell P34', LEDGER)
    def test_simultaneous_press_single_action(self):
        self.assertIn('var action_taken := false', SRC)
        self.assertEqual(SRC.count('if not action_taken:'),2)
        self.assertEqual(SRC.count('action_taken = true'),2)
        self.assertIn('trigger_down[side] = pressed',SRC)
    def test_hover_visual_label(self):
        self.assertIn('action.to_upper()',SRC)
        self.assertIn('labels["control_hint"].text',SRC)
    def test_signature_conflict_never_removes_without_permission(self):
        self.assertIn('INSTALL_FAILED_UPDATE_INCOMPATIBLE',INSTALL)
        self.assertIn("$confirmation -cne 'REPLACE'",INSTALL)
        self.assertLess(INSTALL.index("$confirmation -cne 'REPLACE'"),INSTALL.index("$adb -s $deviceSerial uninstall"))
    def test_version_and_public_release(self):
        self.assertIn('version/code=7',PRESET)
        self.assertIn('version/name="0.7.0"',PRESET)
        self.assertIn('package/unique_name="com.gptimemachine.quest3"',PRESET)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.7_Easy_Install',WORKFLOW)
    def test_unverified_mr_remains_disclosed(self):
        readme=(ROOT/'README.md').read_text()
        self.assertIn('MR unverified',readme)
        self.assertIn('not yet bundled',readme)

if __name__=='__main__': unittest.main()
