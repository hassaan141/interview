# 07 — Trees

## Recognise it

Anything hierarchical: a binary tree, a BST, a file system, a scene graph, a behaviour
tree. The question is almost always "which traversal, and what do I carry down vs. return
up?"

## Traversals — know all four and what each is for

```cpp
// DFS, recursive. The ORDER of the visit relative to the recursion is the whole thing.
void preorder (Node* n)  { if (!n) return; visit(n); preorder(n->l);  preorder(n->r);  }
void inorder  (Node* n)  { if (!n) return; inorder(n->l); visit(n); inorder(n->r);  }
void postorder(Node* n)  { if (!n) return; postorder(n->l); postorder(n->r); visit(n); }

// BFS, level order: a QUEUE, and the "process one level at a time" trick.
std::queue<Node*> q; q.push(root);
while (!q.empty()) {
    const std::size_t level_size = q.size();          // snapshot: this is the level
    for (std::size_t i = 0; i < level_size; ++i) { auto* n = q.front(); q.pop(); ... }
}
```

| Traversal | Use it for |
| --- | --- |
| **preorder** | serialize, copy a tree, "act then descend" |
| **inorder** | **a BST in sorted order** — the single most useful BST fact |
| **postorder** | free/destroy, compute a value from children (height, diameter, sums) |
| **level order (BFS)** | shortest path in an unweighted tree, per-level output, right-side view |

**The recursion shape to internalize**: postorder returns a value *up*, preorder passes
context *down*. Most "hard" tree problems are "return a value up while also updating a
global best" — that is the diameter/max-path-sum pattern.

## BST facts

- Inorder traversal is sorted. Validating a BST is *not* "left < node < right" locally —
  it needs a **(min, max) range carried down**, which is the classic wrong answer.
- Search/insert/delete are O(h): O(log n) balanced, **O(n) degenerate**. `std::map` is a
  red-black tree, so it is always O(log n).
- kth smallest = inorder with a counter, O(h + k).

## Iteration vs. recursion

Recursion is clearer, but the depth is the tree height — a degenerate 10⁶-node tree
overflows the stack. Convert with an explicit `std::vector<Node*>` stack. For a
production autonomy codebase, say: **bound the depth, or iterate**, because a stack
overflow is a SIGSEGV with no diagnosis (course section 03).

## Problems

| File | Problem | Shape |
| --- | --- | --- |
| `01-max-depth.cpp` | Maximum Depth (+ all four traversals) | postorder, and the iterative forms |
| `02-invert-tree.cpp` | Invert Binary Tree | any traversal; the famous one |
| `03-diameter-and-balanced.cpp` | Diameter / Balanced | return a value up, update a global |
| `04-level-order.cpp` | Level Order, Right Side View | BFS with a level snapshot |
| `05-validate-bst.cpp` | Validate BST, Kth Smallest | carry a (min,max) range down |
| `06-lowest-common-ancestor.cpp` | LCA (BST and general tree) | two different algorithms |
| `07-serialize-deserialize.cpp` | Serialize and Deserialize | preorder with null markers |

## Traps

1. Null checks at **every** dereference, including the root.
2. Validating a BST with only local comparisons — wrong; you need the range.
3. In BFS, snapshot `q.size()` **before** the inner loop.
4. Recursion depth on a degenerate tree.
5. Leaking nodes: use `std::unique_ptr` children, or an arena.
6. The "diameter" is a number of **edges**, not nodes — read the problem statement.
