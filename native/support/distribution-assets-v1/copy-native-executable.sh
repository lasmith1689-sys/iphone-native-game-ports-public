#!/bin/sh
set -eu

directory=$(CDPATH='' cd "$(dirname "$0")" && pwd)
input="$directory/Payload/NativeCoD.app/NativeCoD"
expected=$(cat "$directory/link-input.sha256")
actual=$(shasum -a 256 "$input" | cut -d ' ' -f 1)
test "$actual" = "$expected"
output=
while test "$#" -gt 0; do
    if test "$1" = '-o'; then
        test -z "$output" && test "$#" -gt 1
        output="$2"
        shift
    fi
    shift
done
test -n "$output" && test "${output##*/}" = 'NativeCoD'
install -m 0755 "$input" "$output"
cmp "$input" "$output"
