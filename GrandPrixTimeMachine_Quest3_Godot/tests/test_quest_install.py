"""Static regression checks for Quest controller input and the easy-install deliverable."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class QuestInstallationContracts(unittest.TestCase):
    def test_android_etc2_astc_import_is_enabled(self):
        project = (ROOT / "project.godot").read_text()
        self.assertIn("textures/vram_compression/import_etc2_astc=true", project)

    def test_named_controller_trackers(self):
        scene = (ROOT / 'scenes/main.tscn').read_text()
        self.assertIn('tracker = "left_hand"', scene)
        self.assertIn('tracker = "right_hand"', scene)
        self.assertNotIn('/user/hand/', scene)

    def test_aim_pose_for_right_laser(self):
        scene = (ROOT / 'scenes/main.tscn').read_text()
        right = scene.split('[node name="RightController"', 1)[1].split('[node ', 1)[0]
        self.assertIn('pose = "aim"', right)
        source = (ROOT / 'scripts/main.gd').read_text()
        self.assertIn('get_float("trigger")', source)
        self.assertIn('controller.global_transform.basis.z', source)
        self.assertIn('_add_pointer(right_controller, "right")', source)
        self.assertIn('_add_pointer(left_controller, "left")', source)

    def test_cmd_invokes_powershell_script(self):
        cmd = (ROOT / 'install/INSTALL_ON_QUEST3.cmd').read_text()
        self.assertIn('INSTALL_ON_QUEST3.ps1', cmd)
        self.assertIn('-NoProfile -ExecutionPolicy Bypass', cmd)

    def test_installer_requires_actual_apk(self):
        script = (ROOT / 'install/INSTALL_ON_QUEST3.ps1').read_text()
        self.assertIn("Join-Path $root 'GrandPrixTimeMachine_Quest3.apk'", script)
        self.assertIn('Test-Path -LiteralPath $apk -PathType Leaf', script)
        self.assertIn('Use the completed Easy Install download', script)

    def test_verifies_apk_and_package(self):
        script = (ROOT / 'install/INSTALL_ON_QUEST3.ps1').read_text()
        self.assertIn('Get-FileHash', script)
        self.assertIn('SHA256SUMS.txt is missing', script)
        self.assertRegex(script, r'install -r \$apk')
        self.assertIn('pm path com.gptimemachine.quest3', script)
        self.assertIn('unauthorized', script)
        self.assertIn('deviceSerial', script)

    def test_downloads_only_official_google_platform_tools(self):
        script = (ROOT / 'install/INSTALL_ON_QUEST3.ps1').read_text()
        self.assertIn('https://dl.google.com/android/repository/platform-tools-latest-windows.zip', script)
        self.assertNotIn('Invoke-Expression', script)
        self.assertNotIn('Start-Process powershell -Verb RunAs', script)

    def test_fallback_official_meta_and_sidequest(self):
        guide = (ROOT / 'install/READ_ME_FIRST.txt').read_text()
        self.assertIn('developers.meta.com', guide)
        self.assertIn('sidequestvr.com', guide)
        self.assertIn('Unknown Sources', guide)
        self.assertIn('Developer Mode', guide)

if __name__ == '__main__': unittest.main()
