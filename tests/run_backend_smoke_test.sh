#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 6 ]]; then
    echo "Usage: $0 <compiler> <source.si> <work-dir> <nasm> <ld> <expected-exit>" >&2
    exit 2
fi

compiler="$1"
source_file="$2"
work_dir="$3"
nasm_executable="$4"
linker_executable="$5"
expected_exit="$6"

rm -rf "$work_dir"
mkdir -p "$work_dir"

assembly_file="$work_dir/program.asm"
object_file="$work_dir/program.o"
executable_file="$work_dir/program"

"$compiler" "$source_file" "$assembly_file"
"$nasm_executable" -f elf64 "$assembly_file" -o "$object_file"
"$linker_executable" "$object_file" -o "$executable_file"

# The test program intentionally returns 42, which is a non-zero
# process status. Temporarily disable immediate-exit behaviour so
# we can inspect it rather than treating it as a shell failure.
set +e
"$executable_file"
actual_exit=$?
set -e

if [[ "$actual_exit" -ne "$expected_exit" ]]; then
    echo "Expected exit code $expected_exit, got $actual_exit" >&2
    exit 1
fi

echo "Native backend smoke test passed with exit code $actual_exit"