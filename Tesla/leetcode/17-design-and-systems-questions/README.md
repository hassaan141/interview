# 17 — Design and Systems Questions

**This is the most likely section for this interview.** A foundations team is far more
likely to ask you to implement a ring buffer, an object pool, or a rate limiter than to
ask for the longest palindromic substring. Every problem here is a real component you
would find in an autonomy stack.

## What they are actually testing

1. **Can you choose the right data structure** and justify it against alternatives.
2. **Ownership and lifetime**: who allocates, who frees, what happens on an early return.
3. **Complexity of every operation**, including the amortized and worst cases.
4. **Edge cases**: empty, full, capacity 0, wraparound, concurrent access.
5. **The real-time constraints**: no allocation in the hot path, bounded worst case, no
   unbounded blocking. This is where most candidates lose the interview.

## The standard follow-ups — prepare answers to all five

- "Now make it thread safe." → What is the concurrency model? One producer or many? Do
  readers mutate state (an LRU `get` does)? Can you shard instead of locking? (Course
  section 11.)
- "Now make it work without allocating." → Fixed capacity, preallocated storage,
  placement new, an intrusive free list. (Course section 03.)
- "What is the worst-case latency?" → Not the average. Where is the `malloc`, the
  rehash, the reallocation, the lock?
- "How would you test it?" → Boundaries, a randomized model-based test against a
  reference implementation, TSan for the concurrent parts, a fuzz harness for anything
  parsing. (Course section 13.)
- "How would you measure it?" → A benchmark with percentiles, `perf stat` for the
  reason. (Course section 14.)

## Problems

| File | Component | The real-world version |
| --- | --- | --- |
| `01-ring-buffer-spsc.cpp` | lock-free single-producer/single-consumer queue | sensor thread → processing thread handoff |
| `02-object-pool.cpp` | fixed-capacity pool with an intrusive free list | frame buffers, message objects, no `malloc` in the loop |
| `03-rate-limiter.cpp` | token bucket + sliding-window counter | log throttling, command rate limits, back-pressure |
| `04-thread-pool.cpp` | worker pool with futures and clean shutdown | parallel stage execution |
| `05-monotonic-queue-stats.cpp` | O(1) sliding-window min/max, streaming stats | rolling sensor statistics |
| `06-time-based-kv-store.cpp` | versioned key-value store, TTL cache | configuration snapshots, log replay lookups |
| `07-latency-histogram.cpp` | log-bucketed percentile tracker | **the p99 tooling this job posting describes** |

See also `../06-linked-list/06-lru-cache-list.cpp` for the LRU cache, which belongs to
this family.

## How to practise these

Write each one from a blank file, with no reference, in under 25 minutes, then compile it
with `-fsanitize=address,undefined` and run the tests. Then answer the five follow-ups out
loud. These are the problems where saying "this never allocates after construction, and
the worst case is bounded" wins the interview.
