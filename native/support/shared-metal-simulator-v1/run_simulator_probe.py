"""Run only the owned Metal scene fixture on a fresh iOS Simulator."""

import hashlib
import json
import math
from pathlib import Path
import platform
import plistlib
import re
import struct
import subprocess
import sys
import time
from uuid import UUID


SOURCE = Path(__file__).resolve().parent
BUNDLE = 'local.owned.SharedMetalDiagnostic'
EXECUTABLE = 'SharedMetalDiagnostic'
AUTOSTART = '--owned-metal-simulator-autostart-v1'


def command(*args, timeout=90):
    result = subprocess.run([str(value) for value in args], capture_output=True, timeout=timeout)
    if result.stderr:
        print(result.stderr.decode('utf-8', 'replace'), end='', flush=True)
    result.check_returncode()
    return result.stdout


def xcrun(*args, timeout=90):
    return command('/usr/bin/xcrun', *args, timeout=timeout)


def version_tuple(value):
    assert re.fullmatch(r'\d+\.\d+(?:\.\d+)?', value), value
    parts = [int(part) for part in value.split('.')]
    return tuple((parts+[0])[:3])


def version_word(parts):
    major, minor, patch = parts
    return (major << 16) | (minor << 8) | patch


def verify_platform(binary, sdk_version):
    magic, cpu, subtype, kind, count, total, *_ = struct.unpack_from('<8I', binary)
    assert (magic, cpu, subtype, kind) == (0xfeedfacf, 0x100000c, 0, 2)
    assert 0 < count <= 64 and 32+total <= len(binary)
    versions, at = [], 32
    for _ in range(count):
        kind, size = struct.unpack_from('<II', binary, at)
        assert 8 <= size <= 32+total-at
        if kind == 0x32:
            versions.append(struct.unpack_from('<3I', binary, at+8))
        at += size
    assert at == 32+total and versions == [(7, 26 << 16, version_word(version_tuple(sdk_version)))]


def compile_app(work, report):
    sdk = xcrun('--sdk', 'iphonesimulator', '--show-sdk-path').decode().strip()
    sdk_version = xcrun('--sdk', 'iphonesimulator', '--show-sdk-version').decode().strip()
    assert Path(sdk).is_dir() and version_tuple(sdk_version) >= (26, 0, 0)
    report['simulatorSdk'] = sdk_version
    app = work/f'{EXECUTABLE}.app'
    app.mkdir()
    common = ['--sdk', 'iphonesimulator', 'clang', '-target', 'arm64-apple-ios26.0-simulator',
              '-isysroot', sdk, '-O2', '-Wall', '-Wextra', '-Werror']
    policy = work/'policy.o'
    xcrun(*common, '-std=c11', '-c', SOURCE/'shell_policy.c', '-o', policy)
    xcrun(*common, '-fobjc-arc', SOURCE/'main.m', policy, '-framework', 'UIKit',
          '-framework', 'Foundation', '-framework', 'Metal', '-framework', 'QuartzCore',
          '-framework', 'CoreGraphics', '-o', app/EXECUTABLE)
    air = work/'diagnostic.air'
    xcrun('--sdk', 'iphonesimulator', 'metal', '-target', 'air64-apple-ios26.0-simulator',
          '-c', SOURCE/'diagnostic.metal', '-o', air)
    xcrun('--sdk', 'iphonesimulator', 'metallib', air, '-o', app/'default.metallib')
    info = plistlib.loads((SOURCE/'Info.plist').read_bytes())
    assert info['CFBundleIdentifier'] == BUNDLE and info['CFBundleExecutable'] == EXECUTABLE
    info.update(CFBundleSupportedPlatforms=['iPhoneSimulator'], DTPlatformName='iphonesimulator',
                DTSDKName='iphonesimulator'+sdk_version)
    (app/'Info.plist').write_bytes(plistlib.dumps(info))
    verify_platform((app/EXECUTABLE).read_bytes(), sdk_version)
    command('/usr/bin/codesign', '--force', '--sign', '-', app)
    command('/usr/bin/codesign', '--verify', '--strict', app)
    report['simulatorBundle'] = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                                 for path in (app/EXECUTABLE, app/'default.metallib', app/'Info.plist')}
    return app


def select_simulator():
    available = json.loads(xcrun('simctl', 'list', '-j'))
    runtimes = [row for row in available['runtimes'] if row.get('isAvailable') and
                row['identifier'].startswith('com.apple.CoreSimulator.SimRuntime.iOS-') and
                version_tuple(row['version']) >= (26, 0, 0)]
    assert runtimes, 'No installed iOS Simulator runtime supports the fixture'
    runtime = max(runtimes, key=lambda row: version_tuple(row['version']))
    types = {row['name']: row['identifier'] for row in available['devicetypes']}
    for name in ('iPhone 17 Pro', 'iPhone 16 Pro', 'iPhone 16'):
        if name in types:
            return name, types[name], runtime
    raise RuntimeError('No selected iPhone device type is installed')


def read_events(path, proof):
    deadline = time.monotonic()+35
    while time.monotonic() < deadline:
        if path.is_file():
            raw = path.read_bytes()
            assert len(raw) <= 65536
            state = json.loads(raw)
            assert state['schema'] == 'owned-metal-diagnostic-v1' and not state['engineLoaded']
            events = state['events']
            assert 0 < len(events) <= 32
            if any(row['combinedSuccess'] for row in events):
                completed = [r for r in events if r['event'] == 'command-completed-callback']
                presented = [r for r in events if r['event'] == 'drawable-presented-callback']
                assert len(completed) == len(presented) == 1
                assert completed[0]['gpuCompleted'] and completed[0]['samplePixelsMatched']
                timestamp = presented[0]['presentedTime']
                assert presented[0]['presentationValid'] and math.isfinite(timestamp) and timestamp > 0
                (proof/'metal-diagnostic.json').write_bytes(raw)
                return len(events)
            if events[-1]['failed'] or events[-1]['cancelled']:
                (proof/'metal-diagnostic.json').write_bytes(raw)
                raise RuntimeError(f'Simulator fixture stopped: {events[-1]["event"]}')
        time.sleep(0.25)
    if path.is_file():
        (proof/'metal-diagnostic.json').write_bytes(path.read_bytes())
    raise RuntimeError('No successful simulator observation before deadline')


def main():
    assert sys.platform == 'darwin' and platform.machine() == 'arm64'
    work, proof = (Path(argument).resolve() for argument in sys.argv[1:3])
    assert not work.exists() and not proof.exists()
    work.mkdir()
    proof.mkdir()
    report = {'stage': 'compile', 'iphoneExecuted': False, 'gameExecuted': False,
              'simulatorCompletionObserved': False, 'simulatorPresentationObserved': False}
    simulator = None
    booted = False
    try:
        app = compile_app(work, report)
        report['stage'] = 'choose-runtime'
        name, kind, runtime = select_simulator()
        report.update(simulatorDeviceType=name, simulatorRuntime=runtime['identifier'])
        report['stage'] = 'create-and-boot'
        identity = xcrun('simctl', 'create', 'OwnedMetalFixture', kind, runtime['identifier']).decode().strip()
        simulator = str(UUID(identity)).upper()
        xcrun('simctl', 'bootstatus', simulator, '-b', timeout=225)
        booted = True
        report['stage'] = 'install-and-launch'
        xcrun('simctl', 'install', simulator, app)
        xcrun('simctl', 'launch', simulator, BUNDLE, AUTOSTART)
        container = Path(xcrun('simctl', 'get_app_container', simulator, BUNDLE, 'data').decode().strip())
        assert container.is_absolute() and container.is_dir()
        report['stage'] = 'read-observation'
        count = read_events(container/'Documents/metal-diagnostic.json', proof)
        report.update(eventCount=count, simulatorCompletionObserved=True, simulatorPresentationObserved=True,
                      stage='observed')
        xcrun('simctl', 'io', simulator, 'screenshot', proof/'simulator.png', timeout=15)
        assert (proof/'simulator.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
        print('PASS owned simulator foreground render, samples and separate presentation callback')
    except Exception as error:
        report['error'] = type(error).__name__
        raise
    finally:
        try:
            if simulator:
                try:
                    if booted:
                        xcrun('simctl', 'shutdown', simulator, timeout=25)
                finally:
                    xcrun('simctl', 'delete', simulator, timeout=25)
                    report['freshSimulatorDeleted'] = True
        finally:
            (proof/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
