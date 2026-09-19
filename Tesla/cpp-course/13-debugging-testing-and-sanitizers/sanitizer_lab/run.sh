#!/usr/bin/env bash
# Watch each sanitizer catch its own class of bug. Nothing here should "pass".
set -u
cd "$(dirname "$0")"
CXX=${CXX:-g++}
BASE="-std=c++20 -O1 -g -fno-omit-frame-pointer"
OUT=$(mktemp -d); trap 'rm -rf "$OUT"' EXIT

hdr() { printf '\n──────── %s ────────\n' "$1"; }
# Print the sanitizer verdict line plus the first frame of the report.
report() { grep -m1 -E 'ERROR|runtime error|WARNING|SUMMARY|CHECK failed' || echo "  (no diagnostic)"; }

hdr "ASan + UBSan build"
$CXX $BASE -fsanitize=address,undefined -pthread bugs.cpp -o "$OUT/asan" 2>/dev/null
for case in heap_overflow use_after_free use_after_return stack_overflow leak \
            signed_overflow bad_shift misaligned uninitialized; do
    printf '%-20s -> ' "$case"
    ASAN_OPTIONS=detect_leaks=1:abort_on_error=0 "$OUT/asan" "$case" 2>&1 | report
done

hdr "TSan build (races and lock-order inversion)"
$CXX $BASE -fsanitize=thread -pthread bugs.cpp -o "$OUT/tsan" 2>/dev/null
for case in data_race lock_inversion; do
    printf '%-20s -> ' "$case"
    TSAN_OPTIONS=halt_on_error=0 "$OUT/tsan" "$case" 2>&1 | report
done

hdr "Plain -O2 build: the SAME bugs, mostly silent"
$CXX -std=c++20 -O2 -w -pthread bugs.cpp -o "$OUT/plain" 2>/dev/null
for case in heap_overflow signed_overflow uninitialized data_race; do
    printf '%-20s -> ' "$case"
    "$OUT/plain" "$case" 2>&1 | head -1
done
echo "  ^ THIS is the lesson: without sanitizers these look like they work."

hdr "Cheap always-on hardening (no sanitizer)"
$CXX -std=c++20 -O2 -D_GLIBCXX_ASSERTIONS -D_FORTIFY_SOURCE=3 \
     -fstack-protector-strong -ftrivial-auto-var-init=zero -w -pthread \
     bugs.cpp -o "$OUT/hardened" 2>/dev/null
printf '%-20s -> ' "uninitialized"
"$OUT/hardened" uninitialized 2>&1 | head -1
echo "  ^ -ftrivial-auto-var-init=zero makes the uninitialized read DETERMINISTIC"
echo "    (zeros instead of garbage) for ~0-2% cost. Worth it in production."

hdr "Compiler warnings alone catch some of them, for free"
$CXX -std=c++20 -O2 -Wall -Wextra -c bugs.cpp -o /dev/null 2>&1 \
  | grep -E 'warning' | head -6

hdr "An invariant violation, for EXPECT_DEATH-style tests"
printf '%-20s -> ' "check_failure"
"$OUT/plain" check_failure 2>&1 | report

hdr "done"
