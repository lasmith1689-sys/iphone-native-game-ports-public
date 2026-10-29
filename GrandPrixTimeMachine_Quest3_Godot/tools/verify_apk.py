#!/usr/bin/env python3
"""Integrity checks for an exported standalone Android ARM64 APK.

Checks archive structure and target architecture; does *not* prove Quest
startup, signing validity, passthrough functionality or frame rate. Use
Android SDK apksigner/adb for signing and device-side validation.
"""
import hashlib
import json
import sys
from pathlib import Path
from zipfile import BadZipFile, ZipFile


def verify(apk: Path) -> dict:
    if not apk.exists() or not apk.is_file():
        raise ValueError(f'APK missing: {apk}')
    if apk.stat().st_size < 2_000_000:
        raise ValueError(f'APK unexpectedly small: {apk.stat().st_size} bytes')
    with apk.open('rb') as fh:
        digest = hashlib.file_digest(fh, 'sha256').hexdigest()
    try:
        with ZipFile(apk) as archive:
            if archive.testzip() is not None:
                raise ValueError('APK contains a corrupt ZIP member')
            contents = archive.namelist()
            required = ['AndroidManifest.xml', 'classes.dex', 'resources.arsc']
            missing = [f for f in required if f not in contents]
            if missing:
                raise ValueError(f'Android APK entries missing: {missing}')
            native = [f for f in contents if f.startswith('lib/') and f.endswith('.so')]
            arm64 = [f for f in native if f.startswith('lib/arm64-v8a/')]
            other = [f for f in native if not f.startswith('lib/arm64-v8a/')]
            if not arm64:
                raise ValueError('No arm64-v8a native binaries found in APK')
            if other:
                raise ValueError(f'Unexpected non-ARM64 ABI libraries: {other[:4]}')
            if sum(archive.getinfo(f).file_size for f in arm64) < 1_000_000:
                raise ValueError('Native ARM64 library payload suspiciously small')
    except BadZipFile as err:
        raise ValueError('APK is not a valid ZIP-format Android package') from err
    return {
        'apk': apk.name,
        'sha256': digest,
        'bytes': apk.stat().st_size,
        'arm64_libraries': arm64,
        'zip_integrity': 'passed',
        'android_manifest_present': True,
        'signing_verified': False,
        'quest_runtime_tested': False,
    }


def main() -> None:
    if len(sys.argv) != 2:
        print('Usage: python tools/verify_apk.py build/GrandPrixTimeMachine_Quest3.apk', file=sys.stderr)
        sys.exit(2)
    try:
        result = verify(Path(sys.argv[1]))
        print(json.dumps(result, indent=2))
    except ValueError as exc:
        print(f'APK QA failed: {exc}', file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
