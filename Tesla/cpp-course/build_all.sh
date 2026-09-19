#!/usr/bin/env bash
# Compile and run every examples.cpp under Tesla/cpp-course.
set -u
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -O2 -g -pthread"
pass=0; fail=0; failed=()

for f in */examples.cpp; do
    out=$(mktemp)
    if ! $CXX $FLAGS "$f" -o "$out" 2>/tmp/cc_err; then
        printf 'BUILD FAIL  %s\n' "$f"; sed 's/^/    /' /tmp/cc_err | head -6
        fail=$((fail+1)); failed+=("$f"); rm -f "$out"; continue
    fi
    if ! "$out" >/tmp/run_out 2>&1; then
        printf 'RUN FAIL    %s\n' "$f"; sed 's/^/    /' /tmp/run_out | head -8
        fail=$((fail+1)); failed+=("$f")
    else
        printf '  ok  %s\n' "$f"
        pass=$((pass+1))
    fi
    rm -f "$out"
done

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ] || { printf 'failures:\n'; printf '  %s\n' "${failed[@]}"; exit 1; }
