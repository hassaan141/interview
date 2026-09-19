#!/usr/bin/env bash
# Compile and run every solution under Tesla/leetcode with sanitizers on.
set -u
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++20 -Wall -Wextra -g -fsanitize=address,undefined"
pass=0; fail=0; failed=()

for f in */[0-9]*.cpp; do
    out=$(mktemp)
    if ! $CXX $FLAGS "$f" -o "$out" 2>/tmp/cc_err; then
        printf 'BUILD FAIL  %s\n' "$f"; sed 's/^/    /' /tmp/cc_err | head -6
        fail=$((fail+1)); failed+=("$f"); rm -f "$out"; continue
    fi
    if ! "$out" >/tmp/run_out 2>&1; then
        printf 'RUN FAIL    %s\n' "$f"; sed 's/^/    /' /tmp/run_out | head -8
        fail=$((fail+1)); failed+=("$f")
    else
        pass=$((pass+1))
    fi
    rm -f "$out"
done

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ] || { printf 'failures:\n'; printf '  %s\n' "${failed[@]}"; exit 1; }
