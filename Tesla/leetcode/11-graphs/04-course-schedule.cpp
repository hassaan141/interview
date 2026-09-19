// PROBLEM     (a) Can all n courses be finished given prerequisite pairs? (Is the directed
//             graph acyclic?)  (b) Return a valid order.
// APPROACH    TOPOLOGICAL SORT, and you should know both algorithms:
//             (1) KAHN (BFS): compute in-degrees, seed a queue with the zero-in-degree
//                 nodes, and repeatedly remove one and decrement its successors. If fewer
//                 than n nodes come out, a cycle exists -- cycle detection is FREE.
//             (2) DFS: postorder then reverse. Cycle detection needs THREE colours:
//                 white (unvisited), grey (on the current recursion stack), black (done).
//                 A grey neighbour is a back edge, i.e. a cycle. A plain visited[] boolean
//                 CANNOT distinguish "already finished" from "currently in progress" --
//                 that is the classic wrong answer, and the interviewer is watching for it.
// COMPLEXITY  Both O(V + E) time, O(V) space.
// FOLLOW-UPS  Why is this the most job-relevant graph problem? -> it is build-dependency
//             ordering, task scheduling, and initialization ordering (course section 08's
//             static-init fiasco is the same shape).
//             Lexicographically smallest order? -> Kahn with a priority_queue.
//             Parallel schedule / minimum time? -> the longest path in the DAG, which is a
//             DP over the topological order. Worth naming.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

// (1) Kahn: BFS with in-degrees. Returns an empty vector iff a cycle exists.
std::vector<int> topological_order(int n, const std::vector<std::pair<int, int>>& edges) {
    std::vector<std::vector<int>> adj(static_cast<std::size_t>(n));
    std::vector<int> in_degree(static_cast<std::size_t>(n), 0);
    for (const auto& [from, to] : edges) {               // from must precede to
        adj[static_cast<std::size_t>(from)].push_back(to);
        ++in_degree[static_cast<std::size_t>(to)];
    }

    std::queue<int> q;
    for (int i = 0; i < n; ++i)
        if (in_degree[static_cast<std::size_t>(i)] == 0) q.push(i);

    std::vector<int> order;
    order.reserve(static_cast<std::size_t>(n));
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        order.push_back(u);
        for (int v : adj[static_cast<std::size_t>(u)])
            if (--in_degree[static_cast<std::size_t>(v)] == 0) q.push(v);
    }
    if (static_cast<int>(order.size()) != n) return {};   // a cycle blocked some nodes
    return order;
}

bool can_finish(int n, const std::vector<std::pair<int, int>>& edges) {
    return !topological_order(n, edges).empty() || n == 0;
}

// (2) DFS with three colours. Same result, different shape.
namespace {
enum class Colour : std::uint8_t { White, Grey, Black };

bool dfs_order(int u, const std::vector<std::vector<int>>& adj, std::vector<Colour>& colour,
               std::vector<int>& reverse_order) {
    colour[static_cast<std::size_t>(u)] = Colour::Grey;           // on the current path
    for (int v : adj[static_cast<std::size_t>(u)]) {
        const Colour c = colour[static_cast<std::size_t>(v)];
        if (c == Colour::Grey) return false;                       // BACK EDGE -> cycle
        if (c == Colour::White && !dfs_order(v, adj, colour, reverse_order)) return false;
    }
    colour[static_cast<std::size_t>(u)] = Colour::Black;           // finished
    reverse_order.push_back(u);                                    // postorder
    return true;
}
}  // namespace

std::vector<int> topological_order_dfs(int n, const std::vector<std::pair<int, int>>& edges) {
    std::vector<std::vector<int>> adj(static_cast<std::size_t>(n));
    for (const auto& [from, to] : edges) adj[static_cast<std::size_t>(from)].push_back(to);

    std::vector<Colour> colour(static_cast<std::size_t>(n), Colour::White);
    std::vector<int> reverse_order;
    for (int i = 0; i < n; ++i)
        if (colour[static_cast<std::size_t>(i)] == Colour::White)
            if (!dfs_order(i, adj, colour, reverse_order)) return {};
    std::ranges::reverse(reverse_order);                            // postorder reversed
    return reverse_order;
}

// Verify an order actually satisfies every edge.
static bool is_valid_order(int n, const std::vector<std::pair<int, int>>& edges,
                           const std::vector<int>& order) {
    if (static_cast<int>(order.size()) != n) return false;
    std::vector<int> position(static_cast<std::size_t>(n), -1);
    for (std::size_t i = 0; i < order.size(); ++i)
        position[static_cast<std::size_t>(order[i])] = static_cast<int>(i);
    for (const auto& [from, to] : edges)
        if (position[static_cast<std::size_t>(from)] >= position[static_cast<std::size_t>(to)])
            return false;
    return true;
}

int main() {
    assert(can_finish(2, {{1, 0}}));                         // 1 before 0
    assert(!can_finish(2, {{1, 0}, {0, 1}}));                // a 2-cycle
    assert(can_finish(1, {}));
    assert(!can_finish(1, {{0, 0}}));                        // a self loop IS a cycle
    assert(can_finish(4, {{1, 0}, {2, 0}, {3, 1}, {3, 2}}));  // a diamond

    const std::vector<std::pair<int, int>> edges{{0, 1}, {0, 2}, {1, 3}, {2, 3}};
    assert(is_valid_order(4, edges, topological_order(4, edges)));
    assert(is_valid_order(4, edges, topological_order_dfs(4, edges)));

    // Both algorithms must agree on cyclicity.
    const std::vector<std::pair<int, int>> cyclic{{0, 1}, {1, 2}, {2, 0}};
    assert(topological_order(3, cyclic).empty());
    assert(topological_order_dfs(3, cyclic).empty());

    // Disconnected components are fine.
    assert(topological_order(4, {{0, 1}, {2, 3}}).size() == 4);
    assert(topological_order(3, {}).size() == 3);            // no edges: any order works
    return 0;
}
