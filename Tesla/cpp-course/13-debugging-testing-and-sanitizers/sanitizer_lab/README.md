# sanitizer_lab — watch each sanitizer catch its own bug

```bash
bash run.sh
```

`bugs.cpp` contains one function per defect class. `run.sh` builds it three ways and
shows what each build reports:

| Case | Caught by |
| --- | --- |
| `heap_overflow`, `use_after_free`, `stack_overflow`, `leak` | **ASan** (+LSan) |
| `use_after_return` | ASan / UBSan / `-Wreturn-local-addr` |
| `signed_overflow`, `bad_shift`, `misaligned` | **UBSan** |
| `data_race`, `lock_inversion` | **TSan** |
| `uninitialized` | MSan (clang), `-Wuninitialized`, `-ftrivial-auto-var-init` |
| `check_failure` | your own always-on `CHECK` |

The most important section of the output is **"Plain -O2 build: the SAME bugs, mostly
silent"** — the heap overflow writes happily, the signed overflow prints a wrapped
value, and the data race produces the *correct* answer on this run. That is what a
production build without sanitizers looks like, and why they belong in CI rather than
in a debugging session.

Notes:
- ASan and TSan **cannot be combined** — they need separate builds, hence separate CI
  jobs.
- Sanitizers only see code that **executes**, which is why they are paired with good
  tests and fuzzing (see `../examples.cpp`, `Parser.NeverCrashesOnArbitraryInput`).
- `-ftrivial-auto-var-init=zero` costs ~0-2% and turns an indeterminate read from a
  nondeterministic bug into a deterministic one. Worth enabling in production.
