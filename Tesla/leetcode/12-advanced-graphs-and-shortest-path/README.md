# 12 — Advanced Graphs: Dijkstra, Bellman-Ford, Union-Find, MST

## Pick the right algorithm (this table is the section)

| Situation | Algorithm | Complexity |
| --- | --- | --- |
| unweighted shortest path | **BFS** | O(V + E) |
| all weights equal to 0 or 1 | **0-1 BFS** (deque: push_front for 0, push_back for 1) | O(V + E) |
| non-negative weights | **Dijkstra** (binary heap) | O((V + E) log V) |
| **negative** weights | **Bellman-Ford** | O(V·E) |
| negative weights, ≤ k edges | **Bellman-Ford with k relaxations** | O(k·E) |
| all-pairs, small V | **Floyd-Warshall** | O(V³) |
| minimum spanning tree | **Kruskal** (union-find) or **Prim** (heap) | O(E log E) / O(E log V) |
| incremental connectivity | **union-find** | ~O(1) amortized |
| a DAG | relax in **topological order** | O(V + E) |

**Say the table.** Choosing Dijkstra when there are negative edges, or Bellman-Ford when
there are not, is the mistake this section exists to prevent.

## Dijkstra in C++

```cpp
using Entry = std::pair<long long, int>;                    // {distance, node}
std::priority_queue<Entry, std::vector<Entry>, std::greater<>> pq;   // MIN-heap
std::vector<long long> dist(n, kInf);
dist[src] = 0; pq.emplace(0, src);
while (!pq.empty()) {
    auto [d, u] = pq.top(); pq.pop();
    if (d > dist[u]) continue;                 // LAZY DELETION: skip the stale entry
    for (auto [v, w] : adj[u])
        if (d + w < dist[v]) { dist[v] = d + w; pq.emplace(dist[v], v); }
}
```
`std::priority_queue` has **no decrease-key**, so you push a duplicate and skip stale
entries on pop (`if (d > dist[u]) continue;`). That one line is the idiomatic C++
Dijkstra and forgetting it makes the algorithm quadratic. Distances are `long long`
because summing `int` weights overflows.

**Why Dijkstra needs non-negative weights**: it finalizes a node the first time it is
popped, assuming no later path can be shorter. A negative edge breaks that assumption.

## Union-find (disjoint set union)

```cpp
int find(int x) { while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; } return x; }  // halving
bool unite(int a, int b) { int ra=find(a), rb=find(b); if (ra==rb) return false;
                           if (sz[ra]<sz[rb]) std::swap(ra,rb);
                           p[rb]=ra; sz[ra]+=sz[rb]; return true; }
```
Path compression **plus** union by size/rank gives O(α(n)) amortized — effectively
constant. Write `find` **iteratively**: a recursive one can blow the stack on a
pathological chain.

## Problems

| File | Problem | Algorithm |
| --- | --- | --- |
| `01-network-delay-time.cpp` | Network Delay Time | Dijkstra with lazy deletion |
| `02-cheapest-flights-k-stops.cpp` | Cheapest Flights Within K Stops | Bellman-Ford, k rounds |
| `03-min-cost-connect-points.cpp` | Min Cost to Connect All Points | MST (Prim and Kruskal) |
| `04-word-ladder.cpp` | Word Ladder | BFS on an implicit graph + bidirectional search |
| `05-swim-in-rising-water.cpp` | Swim in Rising Water | Dijkstra variant / binary search + BFS |

## Traps

1. Dijkstra with negative weights — wrong answer, no error.
2. No lazy-deletion check → the heap fills with stale entries.
3. `int` overflow when summing weights — use `long long` and a sentinel below `INT_MAX`.
4. Recursive `find` in union-find.
5. Forgetting union by size/rank — path compression alone is O(log n), not O(α).
6. Bellman-Ford needs a **copy** of the distance array per round when hops are limited,
   or you use an already-relaxed value from the same round.
