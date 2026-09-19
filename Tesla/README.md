# Tesla C++ Interview Prep — Autonomy Systems Foundations

Target role: **Internship, Software Engineer, Autonomy Systems Foundations
(Winter/Spring 2027)** — Tesla Vehicle Software / Autonomy group.

> "You will be building robust code foundations for the autonomy teams to write
> their applications on top of ... developing system tools to benchmark,
> characterize and optimize the latency and throughput of the autonomy
> workloads on the Full-Self-Driving chip ... write tests and integrate with our
> evaluation pipeline."

Read that quote again, because it decides how to study. This is **not** an
application-features role. It is a *foundations / platform / performance* role.
That means the interview weights, roughly:

| Area | Weight | Where it lives here |
| --- | --- | --- |
| Core C++ semantics (memory, lifetime, move, templates, UB) | very high | [`cpp-course/`](cpp-course/README.md) |
| Performance reasoning (cache, SIMD, latency vs. throughput, Amdahl) | very high | [`cpp-course/14-...`](cpp-course/14-performance-architecture-and-optimization/), [`trivia/05-...`](trivia/05-latency-throughput-and-benchmarking/) |
| DS&A coding round in C++ | high | [`leetcode/`](leetcode/README.md) |
| Concurrency + memory model | high | [`cpp-course/11-...`](cpp-course/11-concurrency-and-the-memory-model/), [`trivia/03-...`](trivia/03-concurrency-and-realtime/) |
| Tooling: CMake, sanitizers, perf, gtest | medium-high | [`cpp-course/13-...`](cpp-course/13-debugging-testing-and-sanitizers/), [`cpp-course/16-...`](cpp-course/16-binary-size-and-build-time/) |
| Systems / architecture trivia | medium | [`trivia/`](trivia/README.md) |
| Behavioral + the recruiter's bullet points | medium | [`trivia/09-...`](trivia/09-behavioral-and-recruiter-response/) |

## The three folders

1. **[`cpp-course/`](cpp-course/README.md)** — the *Modern C++ Programming*
   lecture series (Federico Busato, 29 lectures) broken into 16 study sections.
   Each section folder has `README.md` (teach me), `examples.cpp` (run it), and
   `quiz.md` (mock interview questions with answers).
2. **[`leetcode/`](leetcode/README.md)** — 17 pattern folders, each with a
   pattern `README.md` and individual C++ solution files (problem statement,
   approach, complexity, interview follow-ups).
3. **[`trivia/`](trivia/README.md)** — 9 folders of Q&A and trivia aimed
   specifically at this job posting: the FSD chip, latency/throughput tooling,
   real-time constraints, build systems, and the autonomy stack.

## Source note (read this)

- The playlist link you gave resolves to the **Modern C++ Programming** course
  whose 29 slide decks are already in this repo at
  [`../Lang/Modern-CPP-Programming/`](../Lang/Modern-CPP-Programming/). The
  section breakdown in `cpp-course/` maps 1:1 onto those chapters, so you can
  read the slides and my notes side by side. YouTube is blocked from this
  environment, so I could not scrape the exact video titles — I mapped against
  the course's own chapter/topic list instead.
- **You pasted the same playlist URL twice** (once for the lectures, once for
  "these leetcode questions"). There is no LeetCode playlist in that link, so
  `leetcode/` is built on the standard pattern taxonomy (the NeetCode-150 style
  roadmap), written in modern C++ rather than Python. If you meant a specific
  second playlist, send it and I will re-cut `leetcode/` to match its sections.

## How to actually use this (4-week plan)

Assumes ~3 hrs/weekday, ~5 hrs/weekend day. Do **not** read passively — every
section's `quiz.md` is the point.

### Week 1 — stop sucking at the fundamentals
- Day 1-2: `cpp-course/01`, `02`, `03`. Section 03 (memory, pointers,
  references, const) is the single highest-yield folder in this repo.
- Day 3: `cpp-course/04`, `05`.
- Day 4: `cpp-course/06`.
- Day 5: `cpp-course/09` (move semantics — do it early, it unlocks everything).
- Weekend: `leetcode/01`-`04`. 20 problems. Write them by hand first, then
  compile.

### Week 2 — the parts that get you this specific job
- Day 1: `cpp-course/10` (smart pointers, `std::expected`).
- Day 2-3: `cpp-course/11` (concurrency + memory model) + `trivia/03`.
- Day 4-5: `cpp-course/14` (performance) + `trivia/04`, `trivia/05`. Build the
  benchmark harness in `cpp-course/14/examples.cpp` and actually run `perf` on it.
- Weekend: `leetcode/05`-`09`.

### Week 3 — templates, tooling, design
- Day 1-2: `cpp-course/07` (templates, SFINAE, concepts).
- Day 3: `cpp-course/08` (linkage, ODR, libraries) + `cpp-course/16`.
- Day 4: `cpp-course/12` (STL) — know container complexity cold.
- Day 5: `cpp-course/13` (sanitizers, gtest) + `trivia/06`, `trivia/07`.
- Weekend: `leetcode/10`-`14`.

### Week 4 — integrate and rehearse
- Day 1: `cpp-course/15` (design, SOLID, patterns) + `trivia/08`.
- Day 2: `leetcode/15`-`17`. Section 17 (design/systems) is the one most likely
  to show up in *this* interview — ring buffer, LRU, object pool, thread pool.
- Day 3: re-take every `quiz.md` cold. Anything you miss goes on one page.
- Day 4: `trivia/01`, `trivia/02` as rapid-fire drills out loud.
- Day 5: `trivia/09` — write your actual answers to the recruiter's bullets and
  three STAR stories.

## Build everything

```bash
cd Tesla
# one section
g++ -std=c++20 -Wall -Wextra -O2 cpp-course/03-memory-pointers-references-and-const/examples.cpp -o /tmp/ex && /tmp/ex

# a leetcode file
g++ -std=c++20 -Wall -Wextra -g -fsanitize=address,undefined \
    leetcode/01-arrays-and-hashing/01-two-sum.cpp -o /tmp/q && /tmp/q

# everything at once (sanity check)
bash build_all.sh
```

Every `.cpp` file in here is self-contained with a `main()` and asserts that
must pass. If a file does not compile, that is a bug — fix it, that is also
practice.
