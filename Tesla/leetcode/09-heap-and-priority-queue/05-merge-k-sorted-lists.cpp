// PROBLEM     Merge k sorted sequences into one sorted sequence.
// APPROACH    Two optimal-complexity answers, and the comparison is the interview:
//             (a) MIN-HEAP over the k current heads. Pop the smallest, push its successor.
//                 O(N log k) time, O(k) space, and it works on STREAMS (you never need all
//                 N elements in memory) -- that is its real advantage.
//             (b) PAIRWISE (tournament) merging: merge lists 1&2, 3&4, ... then repeat.
//                 Also O(N log k), better constant factors and better cache behaviour,
//                 but it needs all the data up front.
//             The naive "merge one at a time into an accumulator" is O(N*k) -- be ready to
//             explain why: the accumulator is re-walked on every merge.
// COMPLEXITY  Both O(N log k) with N the total number of elements.
// FOLLOW-UPS  Streaming / external sort? -> (a); this is exactly how a k-way merge in an
//             external sort or an LSM-tree compaction works.
//             k enormous? -> (a), since (b) needs all the lists resident.
//             Stability? -> break ties on the source index to keep it stable.

#include <algorithm>
#include <cassert>
#include <queue>
#include <tuple>
#include <vector>

// (a) Heap over the k cursors. The heap entry carries which list it came from.
std::vector<int> merge_k_heap(const std::vector<std::vector<int>>& lists) {
    using Entry = std::tuple<int, std::size_t, std::size_t>;   // {value, list, index}
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;

    std::size_t total = 0;
    for (std::size_t i = 0; i < lists.size(); ++i) {
        total += lists[i].size();
        if (!lists[i].empty()) heap.emplace(lists[i][0], i, 0);
    }
    std::vector<int> out;
    out.reserve(total);
    while (!heap.empty()) {
        const auto [value, list, index] = heap.top();
        heap.pop();
        out.push_back(value);
        if (index + 1 < lists[list].size())                    // advance that cursor
            heap.emplace(lists[list][index + 1], list, index + 1);
    }
    return out;
}

// (b) Pairwise tournament merge: same complexity, better constants.
std::vector<int> merge_k_pairwise(std::vector<std::vector<int>> lists) {
    if (lists.empty()) return {};
    while (lists.size() > 1) {
        std::vector<std::vector<int>> next;
        next.reserve((lists.size() + 1) / 2);
        for (std::size_t i = 0; i < lists.size(); i += 2) {
            if (i + 1 == lists.size()) { next.push_back(std::move(lists[i])); break; }
            std::vector<int> merged;
            merged.reserve(lists[i].size() + lists[i + 1].size());
            std::ranges::merge(lists[i], lists[i + 1], std::back_inserter(merged));
            next.push_back(std::move(merged));
        }
        lists = std::move(next);
    }
    return lists.front();
}

int main() {
    const std::vector<std::vector<int>> lists{{1, 4, 5}, {1, 3, 4}, {2, 6}};
    const std::vector<int> expected{1, 1, 2, 3, 4, 4, 5, 6};
    assert(merge_k_heap(lists) == expected);
    assert(merge_k_pairwise(lists) == expected);

    assert(merge_k_heap({}).empty());
    assert(merge_k_pairwise({}).empty());
    assert(merge_k_heap({{}}).empty());
    assert(merge_k_heap({{}, {}, {}}).empty());
    assert((merge_k_heap({{1}}) == std::vector<int>{1}));
    assert((merge_k_heap({{}, {1, 2}, {}}) == std::vector<int>{1, 2}));   // empty lists
    assert((merge_k_heap({{3}, {2}, {1}}) == std::vector<int>{1, 2, 3}));
    assert((merge_k_heap({{1, 1}, {1, 1}}) == std::vector<int>{1, 1, 1, 1}));
    return 0;
}
