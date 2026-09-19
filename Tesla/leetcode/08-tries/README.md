# 08 — Tries (Prefix Trees)

## Recognise it

- **Prefix** queries: autocomplete, "does any word start with...", longest common prefix.
- Many lookups against a **fixed dictionary**.
- Word search on a grid where a plain DFS per word is too slow.
- IP routing (longest-prefix match) and ASCII protocol dispatch — the systems uses.

## The structure

```cpp
struct Node {
    std::array<std::unique_ptr<Node>, 26> children{};   // dense: O(1) index, 26 pointers
    bool is_word = false;
};
// or, for a sparse/large alphabet:
struct Node {
    std::unordered_map<char, std::unique_ptr<Node>> children;
    bool is_word = false;
};
```

**The trade-off to state**: a 26-slot array is O(1) per character and cache-predictable
but costs 208 bytes per node even for a node with one child; a hash map is O(1) average
with far less memory for sparse data but adds hashing and a pointer chase. For a real
system: a **radix/PATRICIA tree** (compress single-child chains) or a **double-array
trie** — that is what an IP routing table or a fast dictionary actually uses.

## Complexity

| Operation | Trie | `unordered_set<string>` |
| --- | --- | --- |
| insert / search a word of length L | O(L) | O(L) to hash, O(1) buckets |
| **prefix** query | **O(L)** | **O(n·L)** — must scan everything |
| memory | one node per distinct prefix character | one entry per word |

The trie's whole reason to exist is the **prefix** row. If you never do a prefix query, a
hash set is simpler and faster.

## Problems

| File | Problem | Idea |
| --- | --- | --- |
| `01-implement-trie.cpp` | Implement Trie | insert / search / startsWith |
| `02-design-add-search-words.cpp` | Design Add and Search Words | `.` wildcard → DFS over children |
| `03-word-search-ii.cpp` | Word Search II | trie + grid DFS, prune as you go |
| `04-longest-common-prefix.cpp` | Longest Common Prefix | and why you would NOT use a trie |

## Traps

1. Ownership: `unique_ptr` children, or an arena of nodes with `std::uint32_t` indices.
   Raw `new` without a destructor leaks the whole tree.
2. `is_word` vs "has children" — "app" can be a word *and* a prefix of "apple".
3. The alphabet assumption (lowercase a-z) must be stated.
4. In Word Search II, you must mark and un-mark visited cells (backtracking), and you
   should **prune** trie nodes once their word is found or they have no children left —
   without pruning the search revisits dead subtrees.
