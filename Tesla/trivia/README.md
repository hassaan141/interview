# Trivia — questions aimed at *this* job posting

> "As a C++ Software Engineer within the Autonomy group, you will apply your technical
> skills to a variety of system components & foundational code targeting higher
> performance of Self-Driving and Humanoid robot ... building robust code foundations
> for the autonomy teams to write their applications on top of and evangelize best
> software practices ... developing system tools to benchmark, characterize and optimize
> the latency and throughput of the autonomy workloads on the Full-Self-Driving chip ...
> write tests and integrate with our evaluation pipeline."

Every folder here maps to a clause in that paragraph. These are **rapid-fire recall**
drills, not exercises — the goal is to answer out loud, in under 30 seconds, without
hedging.

## Folders

| # | Folder | What it covers | Maps to |
| --- | --- | --- | --- |
| 01 | [`01-cpp-language-rapid-fire`](01-cpp-language-rapid-fire/) | 100 short C++ questions | "C++ Software Engineer" |
| 02 | [`02-memory-and-object-lifetime`](02-memory-and-object-lifetime/) | ownership, lifetime, allocation | "foundational code" |
| 03 | [`03-concurrency-and-realtime`](03-concurrency-and-realtime/) | threads, memory model, RT scheduling | pipeline of threads with deadlines |
| 04 | [`04-computer-architecture-and-fsd-chip`](04-computer-architecture-and-fsd-chip/) | caches, SIMD, ARM, accelerators | "the Full-Self-Driving chip" |
| 05 | [`05-latency-throughput-and-benchmarking`](05-latency-throughput-and-benchmarking/) | measuring and reasoning about performance | "benchmark, characterize and optimize" |
| 06 | [`06-build-systems-toolchain-and-testing`](06-build-systems-toolchain-and-testing/) | CMake, linking, sanitizers, CI | "evangelize best software practices" |
| 07 | [`07-linux-systems-and-debugging`](07-linux-systems-and-debugging/) | OS, syscalls, perf, gdb, core dumps | day-to-day systems work |
| 08 | [`08-autonomy-stack-and-robotics-foundations`](08-autonomy-stack-and-robotics-foundations/) | sensors, middleware, coordinate frames, determinism | "the autonomy teams" |
| 09 | [`09-behavioral-and-recruiter-response`](09-behavioral-and-recruiter-response/) | STAR stories, the recruiter's bullets, questions to ask | the rest of the loop |

## How to drill

1. Cover the answers. Read a question, answer **out loud**, then check.
2. Mark anything you hedged on. Hedging in an interview reads as not knowing.
3. Re-drill only the marked ones the next day.
4. For the technical folders, after each session pick **one** answer and go implement or
   measure it — knowing that a cache miss is ~100 ns is trivia; having measured it in
   `../cpp-course/14-performance-architecture-and-optimization/examples.cpp` is
   experience.

## The honest framing

You will not be asked most of this directly. What the trivia does is make you **fluent**,
so that when the real question comes ("why is this loop slow?", "how would you make this
thread safe?") you reach for the right concept immediately instead of reconstructing it
under pressure. Fluency is the goal, not recall.
