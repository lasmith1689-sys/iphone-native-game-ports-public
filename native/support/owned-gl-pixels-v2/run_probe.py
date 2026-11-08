"""Build owned GL support, then observe actual offscreen Simulator pixels."""
import argparse
import hashlib
import json
import platform
import plistlib
import re
import shlex
import shutil
import struct
import subprocess
import sys
import time
import zipfile
from pathlib import Path
from uuid import UUID

PIN = '2037865590d410bcd7b8d1f22b841472ed6d39ac'
BUNDLE = 'local.owned.GLPixelProbe'
EXECUTABLE = 'GLPixelProbe'
AUTOSTART = '--owned-gl-pixel-probe-v2'
SOURCE = Path(__file__).resolve().parent
COMMANDS = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def version(value):
    assert re.fullmatch(r'\d+\.\d+(?:\.\d+)?', value), value
    return tuple((list(map(int, value.split('.')))+[0])[:3])


def run(label, args, cwd=None, timeout=90):
    argv = list(map(str, args))
    started = time.monotonic()
    result = subprocess.run(argv, cwd=cwd or WORK, capture_output=True, timeout=timeout)
    (PROOF/(label+'.stdout')).write_bytes(result.stdout)
    (PROOF/(label+'.stderr')).write_bytes(result.stderr)
    COMMANDS.append(dict(label=label, argv=argv, cwd=str(cwd or WORK),
                         exitCode=result.returncode, seconds=time.monotonic()-started))
    (PROOF/'commands.json').write_text(json.dumps(COMMANDS, indent=2)+'\n')
    assert result.returncode == 0, f'{label} exited {result.returncode}; see retained logs'
    return result.stdout.decode().strip()


def platform_check(path, sdk):
    raw = path.read_bytes()
    magic, cpu, subtype, kind, count, total, *_ = struct.unpack_from('<8I', raw)
    assert (magic, cpu, subtype, kind) == (0xfeedfacf, 0x100000c, 0, 2)
    assert 0 < count <= 128 and 32+total <= len(raw)
    at, versions = 32, []
    for _ in range(count):
        cmd, size = struct.unpack_from('<II', raw, at)
        assert 8 <= size <= 32+total-at
        if cmd == 0x32:
            versions.append(struct.unpack_from('<3I', raw, at+8))
        at += size
    major, minor, patch = version(sdk)
    assert at == 32+total and versions == [(7, 26 << 16, (major << 16)|(minor << 8)|patch)]


def unpack():
    manifest = json.loads((SOURCE/'source.json').read_bytes())
    assert manifest['format'] == 'owned-gl-simulator-source-v1'
    assert manifest['backendCommit'] == PIN and len(manifest['files']) == 16
    assert digest(SOURCE/'source.zip') == manifest['archiveSha256']
    with zipfile.ZipFile(SOURCE/'source.zip') as archive:
        assert len(archive.infolist()) == len(manifest['files'])
        assert set(archive.namelist()) == set(manifest['files'])
        for row in archive.infolist():
            name = row.filename
            assert not name.startswith('/') and '..' not in name.split('/') and '\\' not in name and ':' not in name
            assert not row.is_dir() and ((row.external_attr >> 16)&0o170000) != 0o120000
            assert row.file_size <= 1024*1024
            raw = archive.read(row)
            assert hashlib.sha256(raw).hexdigest() == manifest['files'][name], name
            target = WORK/name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)
    shutil.copy2(SOURCE/'source.json', PROOF/'source.json')
    return manifest


def compile_app(manifest, expected, report):
    clang = run('clang-path', ['xcrun', '--find', 'clang'])
    sdk = run('sdk-path', ['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-path'])
    sdk_version = run('sdk-version', ['xcrun', '--sdk', 'iphonesimulator', '--show-sdk-version'])
    assert version(sdk_version) == version(expected) and version(sdk_version) >= (26, 0, 0)
    report['sdkVersion'] = sdk_version
    run('xcode-version', ['xcodebuild', '-version'])
    backend = WORK/'backend'
    run('clone', ['git', '-c', 'credential.helper=', '-c', 'core.hooksPath=/dev/null', 'clone',
                 '--no-checkout', 'https://github.com/ptitSeb/gl4es.git', backend], timeout=180)
    run('checkout', ['git', '-C', backend, 'checkout', '--detach', PIN])
    assert run('backend-head', ['git', '-C', backend, 'rev-parse', 'HEAD']) == PIN
    for name, value in manifest['backendPins'].items():
        assert digest(backend/name) == value, name
    aliases = [line.strip() for p in (backend/'src').rglob('*.c') for line in p.read_text().splitlines() if 'AliasDecl(' in line]
    assert sorted(aliases) == sorted([
        'AliasDecl(void,gl4es_glEnableClientStatei,(GLenum array, GLuint index),gl4es_glEnableClientStateIndexed);',
        'AliasDecl(void,gl4es_glDisableClientStatei,(GLenum array, GLuint index),gl4es_glDisableClientStateIndexed);'])
    build = WORK/'backend-build'
    target = 'arm64-apple-ios26.0-simulator'
    run('configure', ['cmake', '-S', backend, '-B', build, '-G', 'Unix Makefiles',
        '-DCMAKE_SYSTEM_NAME=iOS', '-DCMAKE_SYSTEM_PROCESSOR=arm64', '-DCMAKE_OSX_ARCHITECTURES=arm64',
        '-DCMAKE_OSX_SYSROOT='+sdk, '-DCMAKE_OSX_DEPLOYMENT_TARGET=26.0', '-DCMAKE_C_COMPILER='+clang,
        '-DCMAKE_C_COMPILER_TARGET='+target, '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
        '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER', '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_FLAGS=-fstack-usage -include '+str(WORK/'apple_alias_adapter.h'),
        '-DCMAKE_C_FLAGS_RELEASE=-O2 -DNDEBUG', '-DNOX11=ON', '-DNOEGL=ON', '-DSTATICLIB=ON',
        '-DNO_LOADER=ON', '-DNO_INIT_CONSTRUCTOR=ON', '-DDEFAULT_ES=2', '-DGBM=OFF',
        '-DEGL_WRAPPER=OFF', '-DUSE_CCACHE=OFF', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
    run('backend-build', ['cmake', '--build', build, '--target', 'GL', '--parallel', '3'], timeout=900)
    flags = ['-target', target, '-isysroot', sdk, '-O2', '-fvisibility=hidden', '-fstack-usage',
             '-Wall', '-Wextra', '-Werror', '-Wno-error=deprecated-declarations',
             '-I'+str(backend/'include'), '-I'+str(WORK/'v13'), '-I'+str(WORK/'fixture')]
    sources = {name: 'v13/'+name+'.c' for name in
               ('cod_gl_dispatch', 'cod_gl4es_execute', 'cod_gl_compressed_stage', 'cod_gl_owned_profile')}
    sources.update(ngp_dxt_v1='compressed-dxt-v1/ngp_dxt_v1.c', PixelProbe='fixture/PixelProbe.c', main='fixture/main.m')
    objects = []
    for name, relative in sources.items():
        obj = WORK/(name+'.o')
        language = ['-fobjc-arc', '-fblocks'] if relative.endswith('.m') else ['-std=c11', '-fno-fast-math']
        run('compile-'+name, [clang, *flags, *language, '-MMD', '-MF', WORK/(name+'.d'), '-c', WORK/relative, '-o', obj])
        objects.append(obj)
    database = json.loads((build/'compile_commands.json').read_bytes())
    shutil.copy2(build/'compile_commands.json', PROOF/'backend-compile-commands.json')
    row = next(r for r in database if Path(r['file']).resolve() == (backend/'src/gl/glstate.c').resolve())
    matching = list(row.get('arguments') or shlex.split(row['command']))
    matching[matching.index('-c')+1] = str(WORK/'v13/cod_gl4es_unpack.c')
    matching[matching.index('-o')+1] = str(WORK/'cod_gl4es_unpack.o')
    matching += ['-I'+str(backend/'src/gl'), '-MMD', '-MF', str(WORK/'cod_gl4es_unpack.d')]
    run('compile-matching-private-layout', matching, cwd=row['directory'])
    objects.append(WORK/'cod_gl4es_unpack.o')
    app = WORK/(EXECUTABLE+'.app')
    app.mkdir()
    run('link', [clang, '-target', target, '-isysroot', sdk, *objects,
        '-Wl,-force_load,'+str(backend/'lib/libGL.a'), '-framework', 'UIKit', '-framework', 'QuartzCore',
        '-framework', 'CoreGraphics', '-framework', 'OpenGLES', '-framework', 'Foundation', '-framework', 'CoreFoundation',
        '-o', app/EXECUTABLE])
    info = plistlib.loads((WORK/'fixture/Info.plist').read_bytes())
    assert info['CFBundleIdentifier'] == BUNDLE and info['CFBundleExecutable'] == EXECUTABLE
    info.update(CFBundleSupportedPlatforms=['iPhoneSimulator'], DTPlatformName='iphonesimulator', DTSDKName='iphonesimulator'+sdk_version)
    (app/'Info.plist').write_bytes(plistlib.dumps(info))
    platform_check(app/EXECUTABLE, sdk_version)
    run('adhoc-sign', ['/usr/bin/codesign', '--force', '--sign', '-', app])
    run('verify-adhoc', ['/usr/bin/codesign', '--verify', '--strict', app])
    report['objects'] = {p.name: digest(p) for p in objects}
    report['backendLibrarySha256'] = digest(backend/'lib/libGL.a')
    report['appExecutableSha256'] = digest(app/EXECUTABLE)
    for p in WORK.glob('*.su'):
        shutil.copy2(p, PROOF/p.name)
    return app


def verify_observation(state):
    assert state['schema'] == 'owned-gl-pixels-v2' and state['status'] == 'passed'
    assert state['simulator'] is True and state['offscreenOnly'] is True
    assert state['engineLoaded'] is False and state['retailAssetsLoaded'] is False
    assert state['presentationObserved'] is False
    assert state['allSamplesMatch'] is True and state['rgbStagingVerified'] is True
    assert state['phase'] == 3 and state['failureLine'] == state['bridgeStatus'] == 0
    assert state['allocations'] == state['releases'] == 2
    assert state['finishPackets'] == 4 and state['nativeFinishCount'] == 6
    assert state['normalPackets'] == 2 and state['scissorPackets'] == state['orthoPackets'] == state['alphaPackets'] == 1
    packet = state['lastPacket']
    assert packet['name'] == 'glGetError' and packet['token'] == 0xfed40060
    assert packet['argc'] == 0 and packet['words'] == [] and packet['returnSp'] == 0x104
    assert packet['invokeEntered'] is True and packet['executorReturned'] is True
    assert packet['failureStage'] == packet['clientReadStatus'] == 0
    assert packet['inBegin'] == packet['compiling'] == packet['listActive'] == packet['listPending'] == 0
    assert packet['attempt'] == state['packetCount']
    expected = [(1,28,24,[255,0,0,255]), (1,36,24,[0,255,0,255]),
                (1,28,40,[0,0,255,255]), (1,36,40,[255,255,0,255]),
                (1,20,24,[0,0,0,255]), (1,44,24,[0,0,0,255]),
                (1,28,8,[0,0,0,255]), (1,28,56,[0,0,0,255]),
                (2,28,24,[0,0,0,255]), (2,36,24,[0,255,0,255]),
                (2,28,40,[0,0,0,255]), (2,36,40,[255,255,0,255]),
                (3,16,32,[255,255,255,255]), (3,48,32,[0,0,0,255])]
    assert len(state['samples']) == len(expected)
    for actual, (phase,x,y,rgba) in zip(state['samples'], expected):
        assert (actual['phase'],actual['x'],actual['y'],actual['expected']) == (phase,x,y,rgba)
        assert actual['matched'] is True and len(actual['actual']) == 4
        assert all(type(v) is int and 0 <= v <= 255 and abs(v-e) <= 5 for v,e in zip(actual['actual'], rgba))


def observe(path):
    deadline = time.monotonic()+35
    while time.monotonic() < deadline:
        if path.is_file():
            raw = path.read_bytes()
            assert len(raw) <= 65536
            (PROOF/'owned-gl-pixels.json').write_bytes(raw)
            state = json.loads(raw)
            verify_observation(state)
            return
        time.sleep(0.25)
    raise RuntimeError('No terminal offscreen observation within 35 seconds')


def main():
    global WORK, PROOF
    parser = argparse.ArgumentParser()
    parser.add_argument('work', type=Path)
    parser.add_argument('proof', type=Path)
    parser.add_argument('--expected-sdk', default='26.5')
    options = parser.parse_args()
    assert sys.platform == 'darwin' and platform.machine() == 'arm64'
    WORK, PROOF = options.work.resolve(), options.proof.resolve()
    assert not WORK.exists() and not PROOF.exists() and WORK != PROOF
    WORK.mkdir(parents=True); PROOF.mkdir(parents=True)
    report = dict(stage='compile', simulatorPixelsObserved=False, phoneContacted=False,
                  engineExecuted=False, retailAssetsLoaded=False, presentationObserved=False)
    simulator = None
    try:
        app = compile_app(unpack(), options.expected_sdk, report)
        report['stage'] = 'select-runtime'
        inventory = json.loads(run('simulator-list', ['xcrun', 'simctl', 'list', '-j']))
        runtimes = [r for r in inventory['runtimes'] if r.get('isAvailable') and
                    r['identifier'].startswith('com.apple.CoreSimulator.SimRuntime.iOS-') and version(r['version']) >= (26,0,0)]
        assert runtimes, 'No compatible installed iOS Simulator runtime'
        runtime = max(runtimes, key=lambda r: version(r['version']))
        types = {r['name']:r['identifier'] for r in inventory['devicetypes']}
        name = next(n for n in ('iPhone 17 Pro','iPhone 16 Pro','iPhone 16') if n in types)
        report.update(runtime=runtime['identifier'], deviceType=name, stage='boot')
        simulator = str(UUID(run('create', ['xcrun','simctl','create','OwnedGLPixelFixture',types[name],runtime['identifier']]))).upper()
        run('boot', ['xcrun','simctl','bootstatus',simulator,'-b'], timeout=225)
        run('install', ['xcrun','simctl','install',simulator,app])
        run('launch', ['xcrun','simctl','launch',simulator,BUNDLE,AUTOSTART])
        container = Path(run('data-container', ['xcrun','simctl','get_app_container',simulator,BUNDLE,'data']))
        assert container.is_absolute() and container.is_dir()
        report['stage'] = 'observe'
        observe(container/'Documents/owned-gl-pixels.json')
        report.update(stage='passed',simulatorPixelsObserved=True)
        print('PASS actual GL4ES offscreen RGB, clipping, alpha, ortho and opposing-normal lighting pixels')
    except Exception as error:
        report['error'] = type(error).__name__+': '+str(error)
        raise
    finally:
        try:
            if simulator:
                try:
                    run('shutdown', ['xcrun','simctl','shutdown',simulator], timeout=25)
                finally:
                    run('delete-owned-simulator', ['xcrun','simctl','delete',simulator], timeout=25)
                    report['freshSimulatorDeleted'] = True
        finally:
            (PROOF/'result.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
