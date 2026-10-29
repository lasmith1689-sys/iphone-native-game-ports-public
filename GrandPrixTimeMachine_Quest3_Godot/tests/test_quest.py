"""Independent geometry, historical editorial, export and package checks.

These checks are deliberately distinct from a Godot runtime test; no headset or
Unity runtime is implied. Intended to run before the cloud build step.
"""
from pathlib import Path
import math
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / 'scripts' / 'main.gd').read_text()
TRACK = (ROOT / 'scripts' / 'track.gd').read_text()
CONFIG = (ROOT / 'export_presets.cfg').read_text()
PROJECT = (ROOT / 'project.godot').read_text()
CONTROLS = [(-.50,-.19),(-.20,-.23),(.28,-.23),(.50,-.20),(.55,-.07),(.49,.12),(.32,.23),(.02,.26),(-.15,.14),(-.32,.29),(-.53,.18),(-.53,-.02)]

def raw(t):
    x=(t%1.0)*len(CONTROLS); i=int(math.floor(x)); u=x-i; n=len(CONTROLS)
    a,b,c,d=(CONTROLS[j%n] for j in (i-1,i,i+1,i+2))
    return tuple(.5*(2*b[k]+(-a[k]+c[k])*u + (2*a[k]-5*b[k]+4*c[k]-d[k])*u*u + (-a[k]+3*b[k]-3*c[k]+d[k])*u*u*u) for k in range(2))

class VerifyQuest(unittest.TestCase):
    def test_track_closed(self): self.assertLess(math.dist(raw(0),raw(1)),1e-10)
    def test_track_bounds(self):
        for i in range(320):
            x,z=raw(i/320)
            self.assertLess(abs(x),.60); self.assertLess(abs(z),.38)
    def test_track_sampling(self): self.assertIn('SAMPLE_COUNT := 320',TRACK)
    def test_control_points(self): self.assertEqual(len(re.findall(r'Vector2\(',TRACK)),12)
    def test_road_mesh(self): self.assertIn('Mesh.PRIMITIVE_TRIANGLES',SRC)
    def test_track_kerbs(self): self.assertIn('red racing kerbs',SRC)
    def test_car_styles(self):
        for name in ('ANDRETTI','HUNT','LAUDA','DEPAILLER'): self.assertIn(name,SRC)
    def test_historical_six_beats(self): self.assertEqual(len(re.findall(r'"at":', SRC)),6)
    def test_late_race(self): self.assertIn('TYRE TROUBLE',SRC)
    def test_lauda_withdraws(self): self.assertIn('playback < 30.0',SRC)
    def test_title_points(self): self.assertIn('69 POINTS',SRC)
    def test_editorial_disclaimer(self): self.assertIn('NOT LAP ACCURATE',SRC)
    def test_playback_jump(self): self.assertIn('"next"',SRC)
    def test_playback_back(self): self.assertIn('"previous"',SRC)
    def test_speed(self): self.assertIn('speed >= 4.0',SRC)
    def test_scale(self): self.assertIn('5.5 if expanded',SRC)
    def test_recenter(self): self.assertIn('func _recenter()',SRC)
    def test_controller(self): self.assertIn('right_controller.get_float("trigger")',SRC)
    def test_ray_controls(self): self.assertIn('center - (origin + direction * along)',SRC)
    def test_passthrough_fallback(self):
        self.assertIn('XR_ENV_BLEND_MODE_ALPHA_BLEND',SRC)
        self.assertIn('XR_ENV_BLEND_MODE_OPAQUE',SRC)
    def test_xr_initialized(self): self.assertIn('xr_interface.initialize()',SRC)
    def test_no_proprietary_dependency(self):
        self.assertNotIn('MetaXR',SRC)
        self.assertNotIn('OVRManager',SRC)
    def test_android(self): self.assertIn('platform="Android"',CONFIG)
    def test_apk_64bit(self):
        self.assertIn('architectures/arm64-v8a=true',CONFIG)
        self.assertIn('architectures/armeabi-v7a=false',CONFIG)
    def test_openxr_export(self): self.assertIn('xr_features/xr_mode=1',CONFIG)
    def test_native_not_tethered(self): self.assertIn('run/main_scene=',PROJECT)
    def test_no_misleading_claims(self): self.assertIn('Historical BEATS are documented',SRC)
    def test_sources_preserved(self): self.assertTrue((ROOT/'docs'/'HISTORICAL_SOURCES.md').exists())
    def test_files_exist(self):
        for f in ['scenes/main.tscn','scripts/track.gd','scripts/main.gd','export_presets.cfg']:
            with self.subTest(f=f): self.assertTrue((ROOT/f).is_file())
    def test_no_editor_dependencies(self):
        self.assertNotIn('EditorScript',SRC)
        self.assertNotIn('EditorPlugin',SRC)
    def test_yaw_local(self): self.assertIn('car.rotation.y = atan2',SRC)
    def test_retry_xr(self): self.assertIn('call_deferred("_start_xr")',SRC)
    def test_earth_safety_note(self): self.assertIn('REAL FURNITURE', (ROOT/'README.md').read_text().upper())

if __name__=='__main__': unittest.main(verbosity=2)
