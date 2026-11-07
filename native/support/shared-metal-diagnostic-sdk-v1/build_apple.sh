#!/bin/bash
# Fresh unsigned Apple-SDK artifact only. No signing, installation, or network.
set -euo pipefail
if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo 'usage: bash build_apple.sh /absolute/fresh-output-directory [exact-expected-sdk-version; default 27.1]' >&2
  exit 2
fi
src="$(cd "$(dirname "$0")" && pwd)"
out="$1"
expected_sdk="${2:-27.1}"
minimum_os='26.0'
[[ "$out" = /* && ! -e "$out" ]] || { echo 'output must be an absolute, absent path' >&2; exit 2; }
python3 "$src/verify_manifest.py"
sdk="$(xcrun --sdk iphoneos --show-sdk-path)"
sdk_version="$(xcrun --sdk iphoneos --show-sdk-version)"
[[ "$sdk_version" = "$expected_sdk" ]] || { echo "expected pinned iPhoneOS SDK $expected_sdk, got $sdk_version" >&2; exit 2; }
mkdir -p "$out/SharedMetalDiagnostic.app" "$out/objects"
app="$out/SharedMetalDiagnostic.app"
exec > >(tee "$out/build.log") 2>&1
set -x
xcrun --sdk iphoneos clang --version
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -isysroot "$sdk" \
  -fobjc-arc -O2 -Wall -Wextra -Werror -I "$src" -c "$src/main.m" -o "$out/objects/main.o"
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -isysroot "$sdk" \
  -std=c11 -O2 -Wall -Wextra -Werror -pedantic -I "$src" -c "$src/shell_policy.c" -o "$out/objects/shell_policy.o"
xcrun --sdk iphoneos clang -target arm64-apple-ios26.0 -isysroot "$sdk" \
  "$out/objects/main.o" "$out/objects/shell_policy.o" \
  -framework UIKit -framework Foundation -framework Metal -framework QuartzCore -framework CoreGraphics \
  -Wl,-no_adhoc_codesign -Wl,-map,"$out/link.map" -o "$app/SharedMetalDiagnostic"
xcrun --sdk iphoneos metal -target air64-apple-ios26.0 -c "$src/diagnostic.metal" -o "$out/objects/diagnostic.air"
xcrun --sdk iphoneos metallib "$out/objects/diagnostic.air" -o "$app/default.metallib"
cp "$src/Info.plist" "$app/Info.plist"
printf '%s' 'APPL????' > "$app/PkgInfo"
xcrun --sdk iphoneos otool -l "$app/SharedMetalDiagnostic" > "$out/load-commands.txt"
xcrun --sdk iphoneos nm -u "$app/SharedMetalDiagnostic" > "$out/imports.txt"
python3 "$src/apple_receipt.py" "$src" "$out" "$sdk_version" "$expected_sdk" "$minimum_os"
set +x
echo "Unsigned app and compile receipts: $out"
