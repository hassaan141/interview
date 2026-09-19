# 01 — Arrays and Hashing

## Recognise it

- "Have I seen this value/complement/remainder before?" → a hash **map** (value → index)
  or **set** (membership).
- "Group things that share a key" → a map from a canonical key to a bucket.
- "Count occurrences" → a frequency map, or a fixed array when the alphabet is small.
- Any O(n²) brute force that re-scans for a value → replace the inner scan with a lookup.

The whole pattern is **trade memory for time**: one pass building a table, one pass
querying it.

## The C++ toolkit

```cpp
#include <unordered_map>
#include <unordered_set>

std::unordered_map<int, int> seen;       // O(1) average, O(n) worst; NODE-based
seen.reserve(n);                          // avoid rehashing: measurable win
if (auto it = seen.find(x); it != seen.end()) { use(it->second); }
seen.emplace(x, i);                       // does not overwrite
seen[x] = i;                              // inserts a default if absent (careful!)
seen.contains(x);                         // C++20

std::array<int, 26> freq{};                // beats a hash map for a small alphabet
++freq[c - 'a'];                           // no hashing, no allocation, cache-resident
```

Say this in the interview: **for a small fixed alphabet use an array, not a hash map** —
no hashing, no allocation, fits in L1. And `std::unordered_map` is node-based (one
allocation per element, a pointer chase per lookup), so `reserve()` and, in production,
`absl::flat_hash_map`.

## Complexity

| Operation | Average | Worst | Note |
| --- | --- | --- | --- |
| `unordered_map` insert/find | O(1) | O(n) | worst case on hash collisions |
| `map` insert/find | O(log n) | O(log n) | ordered, ~9 cache misses for 500 elements |
| sort | O(n log n) | — | often the simpler answer; say both |
| array-indexed count | O(1) | O(1) | when the key space is small and dense |

## Problems

| File | Problem | Key idea |
| --- | --- | --- |
| `01-two-sum.cpp` | Two Sum | store the complement |
| `02-contains-duplicate.cpp` | Contains Duplicate | a set, and the sort alternative |
| `03-valid-anagram.cpp` | Valid Anagram | fixed-size frequency array |
| `04-group-anagrams.cpp` | Group Anagrams | canonical key → bucket |
| `05-top-k-frequent.cpp` | Top K Frequent Elements | counting + bucket sort, O(n) |
| `06-product-except-self.cpp` | Product of Array Except Self | prefix/suffix passes, no division |
| `07-longest-consecutive.cpp` | Longest Consecutive Sequence | set + only start at sequence heads |
| `08-encode-decode-strings.cpp` | Encode and Decode Strings | length-prefixed framing (a real wire format) |

## Traps

1. `map[key]` **inserts** — use `find`/`contains` to query.
2`reserve()` before a known number of inserts.
3. A frequency array must be sized by the alphabet, and you must state the assumption.
4. Watch integer overflow in sums and products (`long long`, or check).
5. "Sort it" is O(n log n) but zero extra memory and much better cache behaviour —
   mention the trade even when you pick the hash map.
