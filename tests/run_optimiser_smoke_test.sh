#!/usr/bin/env bash
set -euo pipefail

compiler="$1"
source_root="$2"
work_dir="$3"

rm -rf "$work_dir"
mkdir -p "$work_dir/path with spaces"

program="$work_dir/path with spaces/optimised-program"

"$compiler"     "$source_root/tests/test-7.si"     "$program"

if [[ ! -x "$program" ]]; then
    echo "Compiler did not create an executable at: $program" >&2
    exit 1
fi

actual_output="$("$program")"
expected_output="10"

if [[ "$actual_output" != "$expected_output" ]]; then
    echo "Unexpected optimiser smoke-test output:" >&2
    printf '%s
' "$actual_output" >&2
    exit 1
fi

echo "Optimiser native smoke test passed"