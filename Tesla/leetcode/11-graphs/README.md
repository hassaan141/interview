# 11 — Graphs (BFS, DFS, Topological Sort)

## Recognise it

- A **grid** where cells connect to neighbours (islands, flood fill, rotting oranges,
  shortest path in a maze).
- **Dependencies** between items → topological sort (course scheduling, build order — the
  latter is literally your job).
- **Connectivity**: components, cycles, "can I reach X from Y".
- Anything phrased as "shortest number of steps" on an **unweighted** graph → **BFS**.

## Representation

```cpp
// adjacency LIST: the default. O(V + E) memory.
std::vector<std::vector<int>> adj(n);
adj[u].push_back(v);

// adjacency MATRIX: O(V^2) memory, O(1) edge query. Only for dense or small graphs.
std::vector<std::vector<bool>> edge(n, std::vector<bool>(n, false));

// a GRID is an implicit graph -- do not build an adjacency list for it.
constexpr std::array<std::pair<int,int>, 4> kDirs{{{1,0},{-1,0},{0,1},{0,-1}}};
```

## The two traversals

```cpp
// BFS: shortest path in an UNWEIGHTED graph, level by level.
std::queue<int> q; q.push(src); visited[src] = true;
while (!q.empty()) {
    const std::size_t level_size = q.size();          // if you need the distance
    for (std::size_t i = 0; i < level_size; ++i) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) if (!visited[v]) { visited[v] = true; q.push(v); }
    }
    ++distance;
}

// DFS: connectivity, cycles, topological order. Recursive or with an explicit stack.
void dfs(int u) { visited[u] = true; for (int v : adj[u]) if (!visited[v]) dfs(v); }
```

**Mark visited when you ENQUEUE, not when you dequeue.** Marking on dequeue lets the same
node enter the queue many times — the most common BFS bug, and it silently turns O(V+E)
into something much worse.

## Topological sort — both algorithms

```cpp
// Kahn (BFS): repeatedly take a node with in-degree 0.
// If fewer than n nodes come out, there is a CYCLE. That cycle detection is free.

// DFS: postorder, then reverse. Cycle detection needs THREE colours
// (white = unvisited, grey = on the current path, black = done); a grey neighbour is a
// back edge, i.e. a cycle. A plain visited[] flag cannot distinguish "already done" from
// "currently on the stack" -- that is the classic wrong answer.
```

## Complexity

BFS/DFS: **O(V + E)** time, O(V) space. A grid has V = rows·cols and E ≈ 4V, so both are
O(rows·cols). Topological sort is also O(V + E).

## Problems

| File | Problem | Idea |
| --- | --- | --- |
| `01-number-of-islands.cpp` | Number of Islands | grid flood fill, DFS and BFS |
| `02-clone-graph.cpp` | Clone Graph | traversal + a map from old to new |
| `03-rotting-oranges.cpp` | Rotting Oranges | **multi-source** BFS |
| `04-course-schedule.cpp` | Course Schedule I & II | topological sort, both algorithms |
| `05-pacific-atlantic.cpp` | Pacific Atlantic Water Flow | reverse the flow, BFS from the borders |
| `06-graph-valid-tree.cpp` | Graph Valid Tree | connectivity + edge count, and union-find |

## Traps

1. Marking visited on dequeue instead of enqueue.
2. Recursion depth on a large grid — a 1000×1000 all-land grid is a 10⁶-deep DFS, which
   overflows the stack. Use BFS or an explicit stack, and *say so*.
3. Forgetting a bounds check before indexing a neighbour (`r >= rows` catches the
   unsigned wrap for r == -1).
4. A cycle check with only two states instead of three.
5. Mutating the input grid to mark visited is fine and cheap — but say that you are doing
   it, because the caller may not expect it.
