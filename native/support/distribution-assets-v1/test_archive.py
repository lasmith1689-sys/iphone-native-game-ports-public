"""Exercise the archive project with synthetic, SDK-built ARM64 binaries."""

import argparse
import hashlib
from pathlib import Path
import plistlib
import subprocess
from zipfile import ZipFile

from archive_project import FILES, prepare


def run(*args):
    return subprocess.run([str(arg) for arg in args], check=True, timeout=180)


def write_plist(path, info):
    path.write_bytes(plistlib.dumps(info))


def make_fixture(work, catalog):
    source = work / 'fixture'
    source.mkdir()
    identifier = 'org.example.nativearchivefixture'
    framework_names = ('Input', 'CoDGL', 'Foreground')
    linked = []
    clang = ('xcrun', '--sdk', 'iphoneos', 'clang', '-target', 'arm64-apple-ios26.0',
             '-Wl,-no_adhoc_codesign', '-Wall', '-Wextra', '-Werror')
    for index, name in enumerate(framework_names):
        folder = source / 'Frameworks' / (name + '.framework')
        folder.mkdir(parents=True)
        native = work / (name + '.c')
        native.write_text(f'int native_{index}(void) {{ return {index + 1}; }}\n', encoding='ascii')
        output = folder / name
        run(*clang, '-dynamiclib', f'-Wl,-install_name,@rpath/{name}.framework/{name}',
            native, '-o', output)
        write_plist(folder / 'Info.plist', {
            'CFBundleExecutable': name, 'CFBundleIdentifier': identifier + '.' + name.lower(),
            'CFBundlePackageType': 'FMWK', 'CFBundleVersion': '8', 'CFBundleShortVersionString': '0.1',
            'CFBundleSupportedPlatforms': ['iPhoneOS'], 'MinimumOSVersion': '26.0',
        })
        linked.append(output)
    entry = work / 'main.c'
    entry.write_text('int native_0(void);\nint native_1(void);\nint native_2(void);\n'
                     'int main(void) { return native_0() + native_1() + native_2(); }\n',
                     encoding='ascii')
    run(*clang, '-Wl,-rpath,@executable_path/Frameworks', entry, *linked, '-o', source / 'NativeCoD')
    partial = work / 'icons.plist'
    run('xcrun', '--sdk', 'iphoneos', 'actool', '--compile', source, '--platform', 'iphoneos',
        '--minimum-deployment-target', '26.0', '--target-device', 'iphone', '--app-icon', 'AppIcon',
        '--output-partial-info-plist', partial, catalog)
    write_plist(source / 'Info.plist', dict(plistlib.loads(partial.read_bytes()),
        CFBundleExecutable='NativeCoD', CFBundleIdentifier=identifier, CFBundlePackageType='APPL',
        CFBundleShortVersionString='0.1', CFBundleVersion='8', UIDeviceFamily=[1],
        UILaunchScreen={}, UISupportedInterfaceOrientations=['UIInterfaceOrientationLandscapeRight']))
    ipa = work / 'synthetic.ipa'
    with ZipFile(ipa, 'x') as zipped:
        for name in FILES:
            zipped.write(source / name, 'Payload/NativeCoD.app/' + name)
    return ipa, source, identifier, framework_names


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('work', type=Path)
    parser.add_argument('catalog', type=Path)
    args = parser.parse_args()
    work = args.work.resolve()
    work.mkdir()
    ipa, source, identifier, frameworks = make_fixture(work, args.catalog.resolve())
    project = work / 'project'
    prepare(ipa, project, hashlib.sha256(ipa.read_bytes()).hexdigest())
    archive = work / 'Fixture.xcarchive'
    run('xcodebuild', '-project', project / 'NativeCoD.xcodeproj', '-scheme', 'NativeCoD',
        '-destination', 'generic/platform=iOS', '-archivePath', archive,
        '-derivedDataPath', work / 'DerivedData', 'CODE_SIGNING_ALLOWED=NO', 'archive')
    archive_info = plistlib.loads((archive / 'Info.plist').read_bytes())
    properties = archive_info['ApplicationProperties']
    assert properties['ApplicationPath'] == 'Applications/NativeCoD.app'
    assert properties['CFBundleIdentifier'] == identifier
    assert sorted(p.name for p in (archive / 'Products/Applications').iterdir()) == ['NativeCoD.app']
    app = archive / 'Products' / properties['ApplicationPath']
    app_info = plistlib.loads((app / 'Info.plist').read_bytes())
    assert app_info['CFBundleExecutable'] == 'NativeCoD' and app_info['CFBundleVersion'] == '8'
    assert app_info['CFBundleIcons']['CFBundlePrimaryIcon']['CFBundleIconName'] == 'AppIcon'
    assert app_info['UILaunchScreen'] == {}
    for name in FILES:
        assert (app / name).is_file()
        if not name.endswith('/Info.plist') and name != 'Info.plist':
            assert (app / name).read_bytes() == (source / name).read_bytes(), name
    for name in frameworks:
        folder = app / 'Frameworks' / (name + '.framework')
        run('codesign', '--force', '--sign', '-', '--timestamp=none', folder)
    run('codesign', '--force', '--sign', '-', '--timestamp=none', app)
    run('codesign', '--verify', '--deep', '--strict', app)
    print('Synthetic iPhone archive preserved all executable bytes before ad-hoc signing')


if __name__ == '__main__':
    main()
