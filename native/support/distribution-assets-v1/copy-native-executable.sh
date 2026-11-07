#!/bin/sh
set -eu

directory=$(CDPATH='' cd "$(dirname "$0")" && pwd)
input="$directory/Payload/NativeCoD.app/NativeCoD"
(cd "$directory" && shasum -a 256 -c archive-input.sha256)

link_output() {
    result=
    while test "$#" -gt 0; do
        if test "$1" = '-o'; then
            test -z "$result" && test "$#" -gt 1 || return 1
            result="$2"
            shift
        fi
        shift
    done
    test -n "$result" && test "${result##*/}" = 'NativeCoD' || return 1
    printf '%s\n' "$result"
}

output=$(link_output "$@")
# Let the SDK linker produce Xcode's dependency-info sidecar, then substitute
# the verified, prelinked game executable before the separate signing step.
xcrun --sdk iphoneos clang "$@"
install -m 0755 "$input" "$output"
cmp "$input" "$output"
