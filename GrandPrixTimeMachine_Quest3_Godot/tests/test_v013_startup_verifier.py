"""Optional Quest USB startup verifier: privacy and evidence contract."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
PS1 = (ROOT / "install/VERIFY_QUEST3_STARTUP.ps1").read_text()
CMD = (ROOT / "install/VERIFY_QUEST3_STARTUP.cmd").read_text()
WORKFLOW = (ROOT.parent / ".github/workflows/grand-prix-quest3.yml").read_text()
SRC = (ROOT / "scripts/main.gd").read_text()
README = (ROOT / "README.md").read_text()
PRESET = (ROOT / "export_presets.cfg").read_text()

class QuestStartupVerifierTests(unittest.TestCase):
    def test_bundled_in_easy_install(self):
        self.assertIn("root/'install/VERIFY_QUEST3_STARTUP.cmd'", WORKFLOW)
        self.assertIn("root/'install/VERIFY_QUEST3_STARTUP.ps1'", WORKFLOW)
        self.assertIn("VERIFY_QUEST3_STARTUP.ps1", CMD)
        self.assertIn("GrandPrixTimeMachine_Quest3_v0.13_Easy_Install", WORKFLOW)

    def test_validates_connected_and_authorized_device(self):
        self.assertIn("unauthorized", PS1)
        self.assertIn("$connected.Count -ne 1", PS1)
        self.assertIn("shell pm path $package", PS1)
        self.assertIn("INSTALL_ON_QUEST3.cmd first", PS1)

    def test_launches_and_observes_real_process_not_xr(self):
        self.assertIn("shell monkey -p $package", PS1)
        self.assertIn("shell pidof $package", PS1)
        self.assertIn("Start-Sleep -Seconds 5", PS1)
        self.assertIn("does not prove OpenXR rendering", PS1)
        self.assertIn("not declared headset-tested", PS1)

    def test_no_hidden_upload_or_destructive_ops(self):
        for forbidden in ("Invoke-WebRequest", "Invoke-RestMethod", "curl", "uninstall", "reboot", "adb tcpip"):
            self.assertNotIn(forbidden, PS1)
        self.assertIn('No device serial, account details, or system-wide logs are collected.', PS1)
        self.assertIn('No automatic upload', README) if 'No automatic upload' in README else self.assertIn('no automatic upload', README)
        self.assertIn('logcat "--pid=$appPid"', PS1)
        self.assertIn("Set-Content -LiteralPath $report", PS1)
        self.assertNotIn(" logcat -d", PS1)

    def test_version_and_sourced_detail(self):
        self.assertIn("version/code=13", PRESET)
        self.assertIn('version/name="0.13.0"', PRESET)
        self.assertIn("Hunt's left-front tyre deflates", SRC)
        self.assertIn("Hunt passes Regazzoni and Jones", SRC)
        self.assertIn("Motor Sport", README)

if __name__ == '__main__':
    unittest.main()
