"""Exercise APK validation with synthetic archives, never called real Quest APKs."""
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZipFile, ZIP_STORED
import os
import unittest

from tools.verify_apk import verify


class PackageVerifierTests(unittest.TestCase):
    def _make(self, root, names, library_bytes=1_300_000):
        apk = Path(root) / 'test.apk'
        # Use random bytes to avoid ZIP compression making a fake APK tiny.
        with ZipFile(apk, 'w', ZIP_STORED) as z:
            for name in names:
                z.writestr(name, os.urandom(library_bytes if name.endswith('.so') else 300_000))
        return apk

    def test_valid_structure_is_not_claimed_headset_tested(self):
        with TemporaryDirectory() as d:
            f = self._make(d, ['AndroidManifest.xml', 'classes.dex', 'resources.arsc', 'lib/arm64-v8a/libgodot_android.so'])
            result = verify(f)
            self.assertEqual(result['zip_integrity'], 'passed')
            self.assertFalse(result['quest_runtime_tested'])
            self.assertFalse(result['signing_verified'])
            self.assertEqual(len(result['sha256']), 64)

    def test_reject_missing_manifest(self):
        with TemporaryDirectory() as d:
            f = self._make(d, ['classes.dex', 'resources.arsc', 'lib/arm64-v8a/libgodot_android.so'])
            with self.assertRaisesRegex(ValueError, 'unexpectedly small|missing'):
                verify(f)

    def test_reject_wrong_abi(self):
        with TemporaryDirectory() as d:
            f = self._make(d, ['AndroidManifest.xml', 'classes.dex', 'resources.arsc', 'lib/x86_64/libgodot_android.so'])
            with self.assertRaisesRegex(ValueError, 'arm64'):
                verify(f)

    def test_reject_multiple_abis(self):
        with TemporaryDirectory() as d:
            f = self._make(d, ['AndroidManifest.xml', 'classes.dex', 'resources.arsc', 'lib/arm64-v8a/libgodot_android.so', 'lib/x86_64/libother.so'])
            with self.assertRaisesRegex(ValueError, 'non-ARM64'):
                verify(f)

    def test_reject_tiny_file(self):
        with TemporaryDirectory() as d:
            f = Path(d) / 'small.apk'
            f.write_bytes(b'PK')
            with self.assertRaisesRegex(ValueError, 'unexpectedly small'):
                verify(f)


if __name__ == '__main__':
    unittest.main(verbosity=2)
