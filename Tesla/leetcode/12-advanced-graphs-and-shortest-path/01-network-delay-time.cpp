// PROBLEM     A directed weighted graph. From node k, how long until ALL nodes receive a
//             signal? -1 if some node is unreachable.
// APPROACH    Single-source shortest paths with non-negative weights -> DIJKSTRA. The
//             answer is the MAXIMUM of the shortest distances (the last node to hear).
//             std::priority_queue has no decrease-key, so use LAZY DELETION: push a new
//             (distance, node) entry whenever you improve a distance, and on pop skip any
//             entry whose distance is worse than the recorded one. Forgetting that check
//             is the classic bug -- the algorithm still terminates but re-expands nodes.
// COMPLEXITY  Time O((V + E) log V), Space O(V + E).
// FOLLOW-UPS  Negative weights? -> Dijkstra is WRONG (it finalizes a node on first pop,
//             assuming nothing shorter can arrive later). Use Bellman-Ford, O(V*E).
//             All weights 1? -> plain BFS, O(V + E), no heap needed.
//             Weights only 0 or 1? -> 0-1 BFS with a deque, also O(V + E).
//             Dense graph? -> an O(V^2) array-scan Dijkstra beats the heap version.
//             Reconstruct the path? -> keep a parent array and walk it back.

#include <algorithm>
#include <cassert>
#include <limits>
#include <queue>
#include <tuple>
#include <vector>

int network_delay_time(const std::vector<std::tuple<int, int, int>>& times, int n, int k) {
    constexpr long long kInf = std::numeric_limits<long long>::max() / 4;   // headroom
    std::vector<std::vector<std::pair<int, int>>> adj(static_cast<std::size_t>(n) + 1);
    for (const auto& [u, v, w] : times) adj[static_cast<std::size_t>(u)].emplace_back(v, w);

    std::vector<long long> dist(static_cast<std::size_t>(n) + 1, kInf);
    dist[static_cast<std::size_t>(k)] = 0;

    using Entry = std::pair<long long, int>;               // {distance, node}
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> pq;   // MIN-heap
    pq.emplace(0, k);

    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[static_cast<std::size_t>(u)]) continue;   // LAZY DELETION: stale entry
        for (const auto& [v, w] : adj[static_cast<std::size_t>(u)]) {
            const long long nd = d + w;
            if (nd < dist[static_cast<std::size_t>(v)]) {
                dist[static_cast<std::size_t>(v)] = nd;
                pq.emplace(nd, v);                              // push a duplicate, no
            }                                                   // decrease-key needed
        }
    }

    long long worst = 0;
    for (int i = 1; i <= n; ++i) {
        if (dist[static_cast<std::size_t>(i)] == kInf) return -1;   // unreachable
        worst = std::max(worst, dist[static_cast<std::size_t>(i)]);
    }
    return static_cast<int>(worst);
}

int main() {
    assert(network_delay_time({{2, 1, 1}, {2, 3, 1}, {3, 4, 1}}, 4, 2) == 2);
    assert(network_delay_time({{1, 2, 1}}, 2, 1) == 1);
    assert(network_delay_time({{1, 2, 1}}, 2, 2) == -1);        // node 1 unreachable
    assert(network_delay_time({}, 1, 1) == 0);                   // already everywhere
    assert(network_delay_time({}, 2, 1) == -1);
    // A shorter multi-hop path must beat a long direct edge.
    assert(network_delay_time({{1, 2, 100}, {1, 3, 1}, {3, 2, 1}}, 3, 1) == 2);
    // Parallel edges: the cheapest wins.
    assert(network_delay_time({{1, 2, 5}, {1, 2, 2}}, 2, 1) == 2);
    return 0;
}
