#!/usr/bin/env bash
set -euo pipefail

compiler="$1"
source_root="$2"
work_dir="$3"

rm -rf "$work_dir"
mkdir -p "$work_dir/path with spaces"

program="$work_dir/path with spaces/control-flow-program"

"$compiler" \
    "$source_root/tests/test-6.si" \
    "$program"

if [[ ! -x "$program" ]]; then
    echo "Compiler did not create an executable at: $program" >&2
    exit 1
fi

actual_output="$("$program")"
expected_output=$'0\n1\n20\n3\n4'

if [[ "$actual_output" != "$expected_output" ]]; then
    echo "Unexpected control-flow output" >&2
    echo "Expected:" >&2
    printf '%s\n' "$expected_output" >&2
    echo "Actual:" >&2
    printf '%s\n' "$actual_output" >&2
    exit 1
fi

echo "Control-flow smoke test passed"
