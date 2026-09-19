#!/usr/bin/env bash
# Multi-TU linkage experiments. Run from anywhere:  bash build.sh
set -u
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++20 -Wall -Wextra -g"
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

hr() { printf '\n=== %s ===\n' "$1"; }

hr "1. A correct build: internal vs. external linkage, inline variables"
$CXX $FLAGS -c a.cpp    -o "$OUT/a.o"
$CXX $FLAGS -c b.cpp    -o "$OUT/b.o"
$CXX $FLAGS -c main.cpp -o "$OUT/main.o"
$CXX "$OUT"/*.o -o "$OUT/app"
"$OUT/app"
echo "  ^ note: g_inline_counter is SHARED (one object) but s_per_tu_copy is NOT"
echo "    (each TU got its own copy, because it is 'static' in a header)."

hr "2. Undefined reference: drop the TU that defines defined_in_a()"
$CXX $FLAGS "$OUT/b.o" "$OUT/main.o" -o "$OUT/broken" 2>&1 | grep -E 'undefined|ld:' | head -4
echo "  ^ 'undefined reference to defined_in_a()' -- declared in the header, defined"
echo "    only in a.cpp, which we did not link."

hr "3. Undefined reference to vtable: comment out Widget's dtor definition"
sed 's|^Widget::~Widget() = default;|// removed on purpose|' a.cpp > "$OUT/a_novtable.cpp"
$CXX $FLAGS -I. -c "$OUT/a_novtable.cpp" -o "$OUT/a_novtable.o"
$CXX $FLAGS "$OUT/a_novtable.o" "$OUT/b.o" "$OUT/main.o" -o "$OUT/broken2" 2>&1 \
  | grep -E 'vtable|undefined' | head -4
echo "  ^ the vtable is emitted in the TU that defines the first non-inline virtual"
echo "    function (the 'key function'); with none defined, nobody emits it."

hr "4. Multiple definition: make the header function non-inline"
sed 's|^inline int inline_square|int inline_square|' shared.hpp > "$OUT/shared.hpp"
cp a.cpp b.cpp main.cpp "$OUT/"
(cd "$OUT" && $CXX -std=c++20 -c a.cpp -o a2.o && $CXX -std=c++20 -c b.cpp -o b2.o \
   && $CXX a2.o b2.o -o dup 2>&1 | grep -E 'multiple definition' | head -3)
echo "  ^ that is exactly what 'inline' prevents."

hr "5. THE ODR VIOLATION: same header, different -D flags"
$CXX $FLAGS -DEXTRA_FIELD -c a.cpp -o "$OUT/a_odr.o"      # sizeof(Frame) == 24
$CXX $FLAGS               -c b.cpp -o "$OUT/b_odr.o"      # sizeof(Frame) == 8
$CXX $FLAGS               -c main.cpp -o "$OUT/main_odr.o"
$CXX "$OUT/a_odr.o" "$OUT/b_odr.o" "$OUT/main_odr.o" -o "$OUT/odr"
echo "  It LINKED CLEANLY. Now look at the sizes each TU believes in:"
"$OUT/odr"
echo "  ^ three TUs, two different layouts for the same type, no diagnostic, and"
echo "    consume_frame() read the WRONG BYTES (expected 3). This is IFNDR:"
echo "    ill-formed, no diagnostic required -- the linker just picked a definition."
echo
echo "  gcc CAN catch many of these at link time with LTO:"
$CXX $FLAGS -flto -DEXTRA_FIELD -c a.cpp -o "$OUT/a_lto.o"
$CXX $FLAGS -flto              -c b.cpp -o "$OUT/b_lto.o"
$CXX $FLAGS -flto              -c main.cpp -o "$OUT/main_lto.o"
$CXX -flto "$OUT/a_lto.o" "$OUT/b_lto.o" "$OUT/main_lto.o" -o "$OUT/odr_lto" 2>&1 \
  | grep -iE 'odr|violates|different' | head -8
echo "  (-Wodr only sees the mismatch because Frame crosses a TU boundary in"
echo "   consume_frame's signature. A type that never crosses one is invisible to it.)"
echo "  ^ -Wodr (implied by -flto) is why LTO is worth turning on in CI."

hr "6. Symbols: what the linker actually sees"
echo "-- defined in a.o:"
nm -C --defined-only --extern-only "$OUT/a.o" | head -8
echo "-- UNDEFINED in b.o (what b.o needs from someone else):"
nm -C -u "$OUT/b.o" | head -8
echo "-- a TU-local (lowercase 't') vs. an exported (uppercase 'T') symbol:"
nm -C "$OUT/a.o" | grep -iE ' t .*(tu_local_helper|defined_in_a)' | head -4

hr "7. Static vs. shared library"
$CXX $FLAGS -c a.cpp -o "$OUT/a.o"
ar rcs "$OUT/liblink.a" "$OUT/a.o" "$OUT/b.o"
$CXX $FLAGS "$OUT/main.o" -L"$OUT" -llink -o "$OUT/app_static" && echo "  static link OK"
$CXX $FLAGS -fPIC -c a.cpp -o "$OUT/a_pic.o"
$CXX $FLAGS -fPIC -c b.cpp -o "$OUT/b_pic.o"
$CXX -shared -o "$OUT/liblink.so" "$OUT/a_pic.o" "$OUT/b_pic.o"
$CXX $FLAGS "$OUT/main.o" -L"$OUT" -llink -Wl,-rpath,"$OUT" -o "$OUT/app_shared" \
  && echo "  shared link OK" && ldd "$OUT/app_shared" | grep liblink
echo "  sizes:"; ls -l "$OUT/app_static" "$OUT/app_shared" | awk '{print "   ", $5, $9}'
echo "  ^ the static binary carries the code; the shared one carries a dependency."

hr "8. Link ORDER matters for static archives"
$CXX $FLAGS -L"$OUT" -llink "$OUT/main.o" -o "$OUT/order" 2>&1 \
  | grep -E 'undefined' | head -3
echo "  ^ -llink BEFORE main.o: the archive was scanned before anything needed it,"
echo "    so nothing was pulled in. Libraries go LAST."

hr "done"
