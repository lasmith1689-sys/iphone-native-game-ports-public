"""Race Director v0.10 regressions, not a physical Quest test."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/'scripts/main.gd').read_text()
LEDGER=(ROOT/'docs/HISTORICAL_SOURCES.md').read_text()
PRESET=(ROOT/'export_presets.cfg').read_text()
WORKFLOW=(ROOT.parent/'.github/workflows/grand-prix-quest3.yml').read_text()
class RaceDirector(unittest.TestCase):
    def test_nine_distinct_moments(self):
        self.assertEqual(SRC.count('"at": '),9)
        self.assertEqual(SRC.count('"weather": '),9)
        self.assertEqual(SRC.count('"stakes": '),9)
    def test_pit_and_weather(self):
        for s in ('LAP 68: HUNT PITS','asphalt_material.roughness','pit_marker.visible = hunt_in_pits','playback >= 90.0 and playback < 103.0'):
            self.assertIn(s,SRC)
    def test_facts_and_disclaimer(self):
        for s in ('4TH ON COUNTBACK','3RD OUTRIGHT','HUNT 69 POINTS','NOT lap-accurate'):
            self.assertIn(s,SRC)
        self.assertIn('motorsportmagazine.com/archive/article/december-1976/34/japanese-grand-prix-4',LEDGER)
    def test_version(self):
        self.assertIn('version/code=13',PRESET)
        self.assertIn('version/name="0.13.0"',PRESET)
        self.assertIn('GrandPrixTimeMachine_Quest3_v0.13_Easy_Install',WORKFLOW)
if __name__=='__main__': unittest.main()
