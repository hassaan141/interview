// PROBLEM     Cheapest price from src to dst using AT MOST k stops (i.e. k+1 edges).
// APPROACH    The hop limit is what rules Dijkstra out: Dijkstra finalizes a node at its
//             globally cheapest distance, which may use too many hops, and a pricier route
//             with fewer hops can be the only valid one.
//             Use BELLMAN-FORD restricted to k+1 rounds: each round relaxes every edge
//             once, so after i rounds `dist` holds the cheapest cost using at most i edges.
//             THE CRITICAL DETAIL: relax from a SNAPSHOT of the previous round's
//             distances. Relaxing in place lets a value updated earlier in the same round
//             be used again, which silently allows more than i hops. That copy is the
//             whole problem.
// COMPLEXITY  Time O(k * E), Space O(V).
// FOLLOW-UPS  Why not Dijkstra? -> as above; a modified Dijkstra keyed on (node, hops)
//             also works but the state space is V*k.
//             Negative cycles? -> Bellman-Ford detects them with one extra round (if
//             anything still relaxes, a negative cycle is reachable).
//             Exactly k stops rather than at most? -> do not carry the previous best
//             forward; track each hop count separately.

#include <algorithm>
#include <cassert>
#include <limits>
#include <tuple>
#include <vector>

int find_cheapest_price(int n, const std::vector<std::tuple<int, int, int>>& flights,
                        int src, int dst, int k) {
    constexpr int kInf = std::numeric_limits<int>::max() / 4;
    std::vector<int> dist(static_cast<std::size_t>(n), kInf);
    dist[static_cast<std::size_t>(src)] = 0;

    for (int round = 0; round <= k; ++round) {          // k stops == k+1 edges
        std::vector<int> prev = dist;                    // THE SNAPSHOT -- do not skip it
        for (const auto& [from, to, price] : flights) {
            if (prev[static_cast<std::size_t>(from)] == kInf) continue;
            dist[static_cast<std::size_t>(to)] =
                std::min(dist[static_cast<std::size_t>(to)],
                         prev[static_cast<std::size_t>(from)] + price);
        }
    }
    return dist[static_cast<std::size_t>(dst)] == kInf ? -1 : dist[static_cast<std::size_t>(dst)];
}

int main() {
    // The canonical case: the cheap route needs 2 stops, so with k=1 the pricier one wins.
    const std::vector<std::tuple<int, int, int>> f{{0, 1, 100}, {1, 2, 100}, {0, 2, 500}};
    assert(find_cheapest_price(3, f, 0, 2, 1) == 200);
    assert(find_cheapest_price(3, f, 0, 2, 0) == 500);   // direct only
    // With 3 nodes and a cheaper 2-hop chain, more stops allowed = cheaper.
    const std::vector<std::tuple<int, int, int>> g{
        {0, 1, 100}, {1, 2, 100}, {2, 3, 100}, {0, 3, 500}};
    assert(find_cheapest_price(4, g, 0, 3, 1) == 500);   // not enough hops for the chain
    assert(find_cheapest_price(4, g, 0, 3, 2) == 300);   // now the chain fits

    assert(find_cheapest_price(2, {}, 0, 1, 5) == -1);   // unreachable
    assert(find_cheapest_price(1, {}, 0, 0, 0) == 0);    // already there
    assert(find_cheapest_price(3, {{0, 1, 5}}, 0, 1, 0) == 5);
    return 0;
}
