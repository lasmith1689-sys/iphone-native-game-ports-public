"""Create an Xcode archive project from the pinned prelinked app."""

import argparse
from pathlib import Path
import hashlib
import json
import plistlib
import re
import shutil
from zipfile import ZipFile


HERE = Path(__file__).resolve().parent
FILES = ('NativeCoD', 'Info.plist', 'Assets.car', 'AppIcon60x60@2x.png', 'AppIcon76x76@2x~ipad.png',
         'Frameworks/Input.framework/Info.plist', 'Frameworks/Input.framework/Input',
         'Frameworks/CoDGL.framework/Info.plist', 'Frameworks/CoDGL.framework/CoDGL',
         'Frameworks/Foreground.framework/Info.plist', 'Frameworks/Foreground.framework/Foreground')
PRIVACY = 'PrivacyInfo.xcprivacy'


def prepare(ipa, out, expected):
    assert not out.exists()
    assert re.fullmatch(r'[a-f0-9]{64}', expected)
    assert hashlib.sha256(ipa.read_bytes()).hexdigest() == expected
    prefix = 'Payload/NativeCoD.app/'
    with ZipFile(ipa) as zipped:
        entries = zipped.infolist()
        names = {i.filename for i in entries}
        included = FILES + (PRIVACY,) if prefix + PRIVACY in names else FILES
        assert len(entries) == len(included) and names == {prefix + p for p in included}
        assert all(0 < i.file_size <= 120 * 1024**2 for i in entries)
        data = {path: zipped.read(prefix + path) for path in included}
    info = plistlib.loads(data['Info.plist'])
    assert info['CFBundleExecutable'] == 'NativeCoD' and info['CFBundleVersion'] in ('8', '9')
    assert (info['CFBundleVersion'] == '9') == (PRIVACY in data)
    bundle_id = info['CFBundleIdentifier']
    assert re.fullmatch(r'[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)+', bundle_id)
    project = out / 'NativeCoD.xcodeproj'
    scheme_dir = project / 'xcshareddata/xcschemes'
    scheme_dir.mkdir(parents=True)
    for path, raw in data.items():
        destination = out / prefix / path
        destination.parent.mkdir(exist_ok=True, parents=True)
        destination.write_bytes(raw)
        if raw.startswith(b'\xcf\xfa\xed\xfe'):
            destination.chmod(0o755)
    text = (HERE / 'project.pbxproj.in').read_text(encoding='utf-8')
    assert text.count('@BUNDLE_ID@') == 1
    text = text.replace('@BUNDLE_ID@', bundle_id)
    text = text.replace('@PRIVACY_REF@', '000000000000000000000017, ' if PRIVACY in data else '')
    text = text.replace('@PRIVACY_BUILD@', ', 000000000000000000000037' if PRIVACY in data else '')
    (project / 'project.pbxproj').write_text(text, encoding='utf-8', newline='\n')
    shutil.copyfile(HERE / 'NativeCoD.xcscheme', scheme_dir / 'NativeCoD.xcscheme')
    shutil.copyfile(HERE / 'copy-native-executable.sh', out / 'copy-native-executable.sh')
    (out / 'copy-native-executable.sh').chmod(0o755)
    (out / 'archive-anchor.c').write_text('int main(void) { return 74; }\n', encoding='ascii')
    binary_hash = hashlib.sha256(data['NativeCoD']).hexdigest()
    (out / 'archive-input.sha256').write_text(''.join(
        f'{hashlib.sha256(raw).hexdigest()}  {prefix}{name}\n' for name, raw in sorted(data.items())),
        encoding='ascii', newline='\n')
    (out / 'archive-input.json').write_text(json.dumps({'ipaSha256': expected, 'mainSha256': binary_hash,
        'files': {name: hashlib.sha256(raw).hexdigest() for name, raw in data.items()}}, indent=2) + '\n',
        encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ipa', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--expected-sha256', required=True)
    args = parser.parse_args()
    prepare(args.ipa.resolve(), args.output.resolve(), args.expected_sha256)


if __name__ == '__main__':
    main()
