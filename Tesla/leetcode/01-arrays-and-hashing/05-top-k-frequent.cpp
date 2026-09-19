// PROBLEM     Return the k most frequent elements of an array, in any order.
// EXAMPLE     nums = [1,1,1,2,2,3], k = 2  ->  [1,2]
// APPROACH    Count with a hash map, then select the top k. Three ways, and knowing WHICH
//             to pick is the point:
//               (a) sort the counts               O(m log m)
//               (b) a size-k min-heap             O(m log k), O(k) space -- best for k << m
//               (c) BUCKET SORT by frequency      O(n) -- the optimal answer, because a
//                   frequency can never exceed n, so there are only n+1 possible buckets
// COMPLEXITY  (c) Time O(n), Space O(n). This is the answer they are looking for.
// FOLLOW-UPS  Streaming, cannot store all counts? -> count-min sketch / space-saving
//             algorithm (approximate heavy hitters). Need them sorted by frequency? ->
//             walk the buckets from the top. k larger than the number of distinct values?
//             -> clamp, and say what you clamped.

#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>
#include <vector>

// (c) Bucket sort by frequency: O(n).
std::vector<int> top_k_frequent(const std::vector<int>& nums, int k) {
    std::unordered_map<int, int> count;
    count.reserve(nums.size());
    for (int n : nums) ++count[n];

    // A value's frequency is in [1, n], so n+1 buckets suffice -- no sorting needed.
    std::vector<std::vector<int>> buckets(nums.size() + 1);
    for (const auto& [value, freq] : count)
        buckets[static_cast<std::size_t>(freq)].push_back(value);

    std::vector<int> out;
    out.reserve(static_cast<std::size_t>(k));
    for (std::size_t freq = buckets.size(); freq-- > 0 && out.size() < static_cast<std::size_t>(k);)
        for (int value : buckets[freq]) {
            out.push_back(value);
            if (out.size() == static_cast<std::size_t>(k)) break;
        }
    return out;
}

// (b) Size-k min-heap: O(m log k) time, O(k) space. The right answer when k << m or when
// the counts arrive as a stream.
std::vector<int> top_k_frequent_heap(const std::vector<int>& nums, int k) {
    std::unordered_map<int, int> count;
    for (int n : nums) ++count[n];

    // MIN-heap on frequency: std::priority_queue is a MAX-heap by default, so we need
    // std::greater<> to pop the least frequent element.
    using Entry = std::pair<int, int>;                     // {frequency, value}
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
    for (const auto& [value, freq] : count) {
        heap.emplace(freq, value);
        if (heap.size() > static_cast<std::size_t>(k)) heap.pop();   // drop the smallest
    }
    std::vector<int> out;
    while (!heap.empty()) { out.push_back(heap.top().second); heap.pop(); }
    return out;
}

static std::vector<int> sorted(std::vector<int> v) { std::ranges::sort(v); return v; }

int main() {
    assert(sorted(top_k_frequent({1, 1, 1, 2, 2, 3}, 2)) == std::vector<int>({1, 2}));
    assert(top_k_frequent({1}, 1) == std::vector<int>({1}));
    assert(sorted(top_k_frequent({1, 2}, 2)) == std::vector<int>({1, 2}));
    assert(top_k_frequent({4, 4, 4, 5, 5, 6}, 1) == std::vector<int>({4}));
    assert(top_k_frequent({}, 1).empty());

    assert(sorted(top_k_frequent_heap({1, 1, 1, 2, 2, 3}, 2)) == std::vector<int>({1, 2}));
    assert(top_k_frequent_heap({7, 7, 8}, 1) == std::vector<int>({7}));
    return 0;
}
