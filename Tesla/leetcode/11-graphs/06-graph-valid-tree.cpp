// PROBLEM     Given n nodes and a list of undirected edges, is the graph a valid tree?
// APPROACH    A graph is a tree iff it is CONNECTED and has exactly n-1 EDGES. Those two
//             conditions together imply acyclicity, so you never need a separate cycle
//             check -- a connected graph with n-1 edges cannot contain a cycle. Checking
//             the edge count first is O(1) and rejects most bad inputs immediately.
//             Two implementations:
//               (a) BFS/DFS from node 0, then verify every node was reached.
//               (b) UNION-FIND: union each edge; if the two endpoints are already in the
//                   same set, that edge closes a cycle -> not a tree.
// COMPLEXITY  (a) O(V + E). (b) O(E * alpha(V)) with path compression and union by rank,
//             which is effectively O(E).
// FOLLOW-UPS  Why is union-find worth knowing? -> it is THE structure for incremental
//             connectivity (edges arriving over time), Kruskal's MST (section 12), and
//             "number of connected components". BFS has to re-run from scratch when an
//             edge is added; union-find is amortized near-constant per edge.
//             Duplicate or self edges? -> a self loop unions a node with itself, which the
//             find check catches immediately.
//             n == 0? -> define it: an empty graph is conventionally not a tree here.

#include <cassert>
#include <numeric>
#include <queue>
#include <utility>
#include <vector>

// Union-find with path compression + union by size: near O(1) amortized.
class UnionFind {
    std::vector<int> parent_, size_;
    int components_;
public:
    explicit UnionFind(int n)
        : parent_(static_cast<std::size_t>(n)), size_(static_cast<std::size_t>(n), 1),
          components_{n} {
        std::iota(parent_.begin(), parent_.end(), 0);
    }
    int find(int x) {                                   // iterative: no stack overflow
        while (parent_[static_cast<std::size_t>(x)] != x) {
            // Path halving: point at the grandparent as we go.
            parent_[static_cast<std::size_t>(x)] =
                parent_[static_cast<std::size_t>(parent_[static_cast<std::size_t>(x)])];
            x = parent_[static_cast<std::size_t>(x)];
        }
        return x;
    }
    bool unite(int a, int b) {                          // false if already connected
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        if (size_[static_cast<std::size_t>(ra)] < size_[static_cast<std::size_t>(rb)])
            std::swap(ra, rb);                          // attach the smaller to the larger
        parent_[static_cast<std::size_t>(rb)] = ra;
        size_[static_cast<std::size_t>(ra)] += size_[static_cast<std::size_t>(rb)];
        --components_;
        return true;
    }
    int components() const noexcept { return components_; }
};

// (b) Union-find: rejects a cycle the moment an edge connects two already-joined nodes.
bool valid_tree_union_find(int n, const std::vector<std::pair<int, int>>& edges) {
    if (n <= 0) return false;
    if (static_cast<int>(edges.size()) != n - 1) return false;   // O(1) rejection
    UnionFind uf{n};
    for (const auto& [a, b] : edges)
        if (!uf.unite(a, b)) return false;                        // this edge made a cycle
    return uf.components() == 1;
}

// (a) BFS connectivity + the edge count.
bool valid_tree_bfs(int n, const std::vector<std::pair<int, int>>& edges) {
    if (n <= 0) return false;
    if (static_cast<int>(edges.size()) != n - 1) return false;
    std::vector<std::vector<int>> adj(static_cast<std::size_t>(n));
    for (const auto& [a, b] : edges) {
        adj[static_cast<std::size_t>(a)].push_back(b);
        adj[static_cast<std::size_t>(b)].push_back(a);
    }
    std::vector<bool> seen(static_cast<std::size_t>(n), false);
    std::queue<int> q;
    q.push(0);
    seen[0] = true;
    int reached = 1;
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (int v : adj[static_cast<std::size_t>(u)])
            if (!seen[static_cast<std::size_t>(v)]) {
                seen[static_cast<std::size_t>(v)] = true;
                ++reached;
                q.push(v);
            }
    }
    return reached == n;
}

// Bonus: number of connected components, the other classic union-find use.
int count_components(int n, const std::vector<std::pair<int, int>>& edges) {
    UnionFind uf{n};
    for (const auto& [a, b] : edges) uf.unite(a, b);
    return uf.components();
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(5, std::vector<std::pair<int, int>>{{0, 1}, {0, 2}, {0, 3}, {1, 4}}));
        assert(!fn(5, std::vector<std::pair<int, int>>{{0, 1}, {1, 2}, {2, 3}, {1, 3}, {1, 4}}));
        assert(fn(1, std::vector<std::pair<int, int>>{}));            // a single node
        assert(!fn(2, std::vector<std::pair<int, int>>{}));            // disconnected
        assert(!fn(4, std::vector<std::pair<int, int>>{{0, 1}, {2, 3}}));  // two components
        assert(!fn(0, std::vector<std::pair<int, int>>{}));
        assert(!fn(1, std::vector<std::pair<int, int>>{{0, 0}}));       // a self loop
        assert(fn(2, std::vector<std::pair<int, int>>{{0, 1}}));
    };
    check([](int n, const std::vector<std::pair<int, int>>& e) { return valid_tree_union_find(n, e); });
    check([](int n, const std::vector<std::pair<int, int>>& e) { return valid_tree_bfs(n, e); });

    assert(count_components(5, {{0, 1}, {1, 2}, {3, 4}}) == 2);
    assert(count_components(5, {}) == 5);
    assert(count_components(1, {}) == 1);
    return 0;
}
