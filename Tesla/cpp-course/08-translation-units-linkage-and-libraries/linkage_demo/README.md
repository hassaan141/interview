# linkage_demo — run the link errors, don't just read about them

```bash
bash build.sh
```

Eight experiments, each printing the real compiler/linker output:

1. A correct multi-TU build. Shows that an `inline` variable is **one** object
   shared by all TUs, while a `static` variable in a header gives every TU its
   **own** copy.
2. `undefined reference to defined_in_a()` — declared in the header, defined in a
   TU that was not linked.
3. `undefined reference to vtable for Widget` — the vtable is emitted by the TU
   that defines the class's *key function* (the first non-inline virtual, usually
   the destructor). Define none and nobody emits it.
4. `multiple definition of inline_square(int)` — what `inline` exists to prevent.
5. **The ODR violation.** Two TUs compile the same header with different `-D`
   flags, so they disagree about `sizeof(Frame)`. It links cleanly, and
   `consume_frame()` returns the wrong answer because it reads `crc` from the wrong
   offset. Then the same build under `-flto`, where `-Wodr` catches it.
6. `nm` output: `T` (exported) vs. `t` (TU-local) vs. `U` (undefined) vs. `W`
   (weak — that is how `inline` is implemented).
7. Static (`.a`) vs. shared (`.so`) linking of the same objects, with sizes and
   `ldd`.
8. Static-archive **link order**: `-llink` before `main.o` pulls in nothing.

Files: `shared.hpp` (the deliberately educational header), `a.cpp`, `b.cpp`,
`main.cpp`, `build.sh`.
