// PROBLEM     (a) nums[i] is the maximum jump length from index i. Can you reach the last
//             index? (b) What is the minimum number of jumps?
// APPROACH    (a) Track the FURTHEST index reachable so far. Walk left to right; if the
//             current index exceeds that reach, you are stuck. Otherwise extend the reach.
//             O(n), O(1).
//             PROOF (stays-ahead): the set of reachable indices is a prefix [0, reach],
//             because if you can reach i you can also reach everything before it. So one
//             scalar fully describes the reachable set.
//             (b) BFS-like level jumping: the indices reachable in exactly k jumps form a
//             contiguous window. Walk the window, computing the furthest index reachable
//             from anywhere in it; when you fall off the end of the window, that is one
//             more jump. O(n), O(1) -- and it is a BFS without a queue, which is the neat
//             part.
// COMPLEXITY  Both O(n) time, O(1) space. The DP formulations are O(n^2).
// FOLLOW-UPS  Why is greedy correct here but DP needed for similar-looking problems? ->
//             because reachability is a PREFIX property. If jumps had costs, or you could
//             jump backwards, that breaks and you need Dijkstra/DP.
//             Jump Game III (arbitrary +/- jumps)? -> a graph, so BFS.
//             Reconstruct the actual jump sequence? -> record which index provided each
//             level's furthest reach.

#include <algorithm>
#include <cassert>
#include <vector>

// (a) Can you reach the end?
bool can_jump(const std::vector<int>& nums) {
    std::size_t reach = 0;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (i > reach) return false;                    // fell into a hole
        reach = std::max(reach, i + static_cast<std::size_t>(nums[i]));
        if (reach + 1 >= nums.size()) return true;      // early exit
    }
    return true;
}

// (b) Minimum jumps: BFS by levels, without a queue.
int jump(const std::vector<int>& nums) {
    if (nums.size() < 2) return 0;
    int jumps = 0;
    std::size_t current_end = 0;                        // end of the current level
    std::size_t furthest = 0;                            // furthest reachable next
    for (std::size_t i = 0; i + 1 < nums.size(); ++i) {
        furthest = std::max(furthest, i + static_cast<std::size_t>(nums[i]));
        if (i == current_end) {                          // exhausted this level
            ++jumps;
            current_end = furthest;
            if (current_end + 1 >= nums.size()) break;
        }
    }
    return jumps;
}

int main() {
    assert(can_jump({2, 3, 1, 1, 4}));
    assert(!can_jump({3, 2, 1, 0, 4}));                 // stuck at the 0
    assert(can_jump({0}));                               // already at the end
    assert(can_jump({}));
    assert(!can_jump({0, 1}));
    assert(can_jump({1, 0}));
    assert(can_jump({2, 0, 0}));
    assert(!can_jump({1, 0, 1, 0}));

    assert(jump({2, 3, 1, 1, 4}) == 2);
    assert(jump({2, 3, 0, 1, 4}) == 2);
    assert(jump({0}) == 0);
    assert(jump({}) == 0);
    assert(jump({1, 2}) == 1);
    assert(jump({1, 1, 1, 1}) == 3);
    assert(jump({5, 1, 1, 1, 1}) == 1);                  // one big jump
    return 0;
}
