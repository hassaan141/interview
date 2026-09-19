# 06 — Linked List

## Why it matters for *this* job

Linked-list questions test **pointer discipline** directly: ownership, null handling,
and not leaking. In a C++ interview at a systems company, the follow-up is always "who
owns the nodes?" — have the answer ready.

## The four techniques

```cpp
// 1. DUMMY HEAD — removes every "is this the first node?" special case.
Node dummy{0, head};
Node* tail = &dummy;
// ... build ...
return dummy.next;

// 2. FAST / SLOW — middle, cycle detection, nth-from-end.
Node* slow = head; Node* fast = head;
while (fast && fast->next) { slow = slow->next; fast = fast->next->next; }
// slow == middle (second middle for even length)

// 3. ITERATIVE REVERSE — three pointers, memorize this exactly.
Node* prev = nullptr;
while (head) { Node* next = head->next; head->next = prev; prev = head; head = next; }
return prev;

// 4. IN-PLACE MERGE — a dummy head plus a tail pointer, comparing two cursors.
```

## Ownership in C++ (the part LeetCode ignores and interviewers do not)

| Model | Pros | Cons |
| --- | --- | --- |
| raw `Node*` + manual `delete` | matches the LeetCode signature | leaks, double frees |
| `std::unique_ptr<Node> next` | no leaks, clear ownership | **recursive destructor blows the stack** on a long list; makes reversal awkward |
| an arena / `std::vector<Node>` + `std::uint32_t` indices | no allocation per node, cache-friendly, trivially serializable | indices are not type-safe |
| `std::forward_list` | it is already in the library | you rarely want a list at all |

For a real autonomy codebase the answer is usually **"do not use a linked list"** —
`std::vector` wins on cache behaviour (course section 12, measured ~8x). Use the arena
version when you genuinely need stable nodes. Say all of this; it is the differentiator.

The `unique_ptr` stack-overflow point is worth stating precisely: destroying a
1,000,000-node `unique_ptr` chain recurses a million frames. The fix is an iterative
destructor that unlinks in a loop.

## Problems

| File | Problem | Technique |
| --- | --- | --- |
| `01-reverse-linked-list.cpp` | Reverse Linked List | three-pointer reverse (+ ownership discussion) |
| `02-merge-two-sorted-lists.cpp` | Merge Two Sorted Lists | dummy head |
| `03-linked-list-cycle.cpp` | Linked List Cycle (+ find the entry) | Floyd's tortoise and hare |
| `04-remove-nth-from-end.cpp` | Remove Nth Node From End | two pointers, n apart |
| `05-reorder-list.cpp` | Reorder List | middle + reverse + merge, composed |
| `06-lru-cache-list.cpp` | LRU Cache | intrusive doubly linked list + hash map |

## Traps

1. Losing the rest of the list — save `next` **before** rewriting a pointer.
2. Not handling empty (`head == nullptr`) and single-node lists.
3. Off-by-one in fast/slow: `while (fast && fast->next)` vs `while (fast->next && fast->next->next)`
   pick which "middle" you want.
4. Leaking removed nodes.
5. Forgetting to null-terminate the new tail after splitting a list.
