"""Inspect the newly compiled unsigned Mach-O and retain truthful SDK metadata."""
import hashlib
import json
import pathlib
import plistlib
import re
import struct
import subprocess
import sys

src, out = (pathlib.Path(value).resolve() for value in sys.argv[1:3])
sdk_version = sys.argv[3]
expected_sdk, expected_minimum = sys.argv[4:6]
def version_number(value):
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+){0,2}", value):
        raise SystemExit("invalid version string")
    parts = [int(x) for x in value.split(".")]
    parts += [0] * (3 - len(parts))
    if parts[0] > 65535 or parts[1] > 255 or parts[2] > 255:
        raise SystemExit("version components out of range")
    return (parts[0] << 16) | (parts[1] << 8) | parts[2]
if sdk_version != expected_sdk:
    raise SystemExit("measured SDK differs from requested exact version")
app = out / "SharedMetalDiagnostic.app"
binary = (app / "SharedMetalDiagnostic").read_bytes()
if len(binary) < 32:
    raise SystemExit("short executable")
magic, cpu, subtype, filetype, ncmds, sizeofcmds, flags, reserved = struct.unpack_from("<8I", binary)
if magic != 0xFEEDFACF or cpu != 0x0100000C or filetype != 2:
    raise SystemExit("expected thin arm64 MH_EXECUTE")
offset = 32
build_versions = []
has_entry = False
dylibs = []
for _ in range(ncmds):
    cmd, size = struct.unpack_from("<2I", binary, offset)
    if size < 8 or offset + size > 32 + sizeofcmds or offset + size > len(binary):
        raise SystemExit("invalid load command bounds")
    if cmd == 0x1D:
        raise SystemExit("unexpected code signature; build must remain unsigned")
    if cmd == 0x80000028:
        has_entry = True
    if cmd == 0x32:
        if size < 24:
            raise SystemExit("short build version")
        platform, minimum, sdk, tools = struct.unpack_from("<4I", binary, offset + 8)
        build_versions.append({"platform": platform, "minimum": minimum, "sdk": sdk, "tools": tools})
    if cmd in (0xC, 0x80000018, 0x8000001F, 0x80000023):
        nameoffset = struct.unpack_from("<I", binary, offset + 8)[0]
        if nameoffset < 24 or nameoffset >= size:
            raise SystemExit("bad dylib name offset")
        name = binary[offset + nameoffset:offset + size].split(b"\0", 1)[0].decode("utf-8")
        if not name.startswith(("/System/Library/Frameworks/", "/usr/lib/")):
            raise SystemExit("non-system load dependency")
        dylibs.append(name)
    offset += size
if offset != 32 + sizeofcmds or not has_entry or len(build_versions) != 1:
    raise SystemExit("unexpected entry/load-command roster")
bv = build_versions[0]
if (bv["platform"], bv["minimum"], bv["sdk"]) != (2, version_number(expected_minimum), version_number(expected_sdk)):
    raise SystemExit(f"unexpected real SDK/minimum/platform: {bv}")
plist_path = app / "Info.plist"
info = plistlib.loads(plist_path.read_bytes())
if info["MinimumOSVersion"] != expected_minimum or info["CFBundleExecutable"] != "SharedMetalDiagnostic":
    raise SystemExit("source plist identity/minimum mismatch")
info.update({"CFBundleSupportedPlatforms": ["iPhoneOS"], "DTPlatformName": "iphoneos",
             "DTPlatformVersion": sdk_version, "DTSDKName": "iphoneos" + sdk_version,
             "DTSDKBuild": subprocess.check_output(["xcrun", "--sdk", "iphoneos", "--show-sdk-build-version"], text=True).strip()})
plist_path.write_bytes(plistlib.dumps(info))
files = {}
for path in sorted(out.rglob("*")):
    if path.is_file() and path.name != "build.log":
        files[str(path.relative_to(out)).replace("\\", "/")] = hashlib.sha256(path.read_bytes()).hexdigest()
receipt = {"schema": "owned-metal-apple-compile-v1", "signed": False,
           "expected_sdk": expected_sdk, "measured_sdk": sdk_version, "minimum_os": expected_minimum,
           "app_created": True, "iphone_executed": False, "gpu_executed": False,
           "runtime_completion_observed": False, "runtime_presentation_observed": False,
           "build_versions": build_versions, "dylibs": dylibs, "files": files,
           "source_manifest": json.loads((src / "source-manifest.json").read_text(encoding="utf-8"))}
(out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(f"PASS thin arm64 unsigned app, iOS {expected_minimum} / SDK {sdk_version}; no runtime claim")
