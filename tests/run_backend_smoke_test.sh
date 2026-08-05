#!/usr/bin/env bash
set -euo pipefail

compiler="$1"
source_root="$2"
work_dir="$3"

rm -rf "$work_dir"
mkdir -p "$work_dir/path with spaces"

program="$work_dir/path with spaces/test-program"
"$compiler" "$source_root/tests/test-5.si" "$program"

actual_output="$("$program")"

if [[ "$actual_output" != "42" ]]; then
    echo "Expected test program to print 42, got: $actual_output" >&2
    exit 1
fi

negative_source="$work_dir/negative.si"
negative_program="$work_dir/path with spaces/negative-program"

cat > "$negative_source" <<'SI'
fn main(): int {
    out(-42);
    out(0);
    return 0;
}
SI

"$compiler" "$negative_source" "$negative_program"
negative_output="$("$negative_program")"

expected_negative=$'-42\n0'

if [[ "$negative_output" != "$expected_negative" ]]; then
    echo "Unexpected signed runtime output:" >&2
    printf '%s\n' "$negative_output" >&2
    exit 1
fi