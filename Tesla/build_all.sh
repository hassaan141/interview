#!/usr/bin/env bash
# Build and run EVERYTHING under Tesla/. This is the one-command sanity check.
set -u
cd "$(dirname "$0")"
rc=0

printf '\n=========== cpp-course examples ===========\n'
bash cpp-course/build_all.sh || rc=1

printf '\n=========== leetcode solutions (ASan+UBSan) ===========\n'
bash leetcode/build_all.sh || rc=1

printf '\n=========== extras ===========\n'
printf 'linkage demo (multi-TU: ODR, vtables, link order):\n'
bash cpp-course/08-translation-units-linkage-and-libraries/linkage_demo/build.sh >/dev/null 2>&1 \
    && printf '  ok\n' || { printf '  FAILED\n'; rc=1; }
printf 'sanitizer lab (each sanitizer catching its own bug class):\n'
bash cpp-course/13-debugging-testing-and-sanitizers/sanitizer_lab/run.sh >/dev/null 2>&1 \
    && printf '  ok\n' || { printf '  FAILED\n'; rc=1; }

printf '\n'
[ "$rc" -eq 0 ] && printf 'ALL GREEN\n' || printf 'SOMETHING FAILED (see above)\n'
exit "$rc"
