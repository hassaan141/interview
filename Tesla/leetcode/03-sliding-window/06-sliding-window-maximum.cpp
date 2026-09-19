// PROBLEM     For every window of size k, report its maximum.
// EXAMPLE     nums = [1,3,-1,-3,5,3,6,7], k = 3 -> [3,3,5,5,6,7]
// APPROACH    MONOTONIC DEQUE of INDICES, kept decreasing by value. On each step:
//               1. pop indices that have fallen out of the window (front)
//               2. pop from the back every index whose value <= the new value -- they can
//                  never be the maximum again, because the new element is bigger AND
//                  stays in the window longer
//               3. push the new index; the front is now the window's maximum
// COMPLEXITY  Time O(n) -- each index is pushed once and popped once, so the inner while
//             is amortized O(1). Space O(k). A max-heap gives O(n log k); a naive
//             re-scan is O(nk).
// FOLLOW-UPS  Why indices and not values? -> you need the index to know when it expires.
//             Sliding window MINIMUM? -> flip the comparison.
//             Sliding window median? -> two heaps, or an ordered multiset, O(n log k).
//             This deque is the same structure used for "next greater element"
//             (section 04) and for the O(n) window-min in DP optimizations.

#include <cassert>
#include <deque>
#include <vector>

std::vector<int> max_sliding_window(const std::vector<int>& nums, std::size_t k) {
    std::vector<int> out;
    if (k == 0 || nums.size() < k) return out;
    out.reserve(nums.size() - k + 1);

    std::deque<std::size_t> dq;                      // indices, values DECREASING
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (!dq.empty() && dq.front() + k <= i) dq.pop_front();       // expired
        while (!dq.empty() && nums[dq.back()] <= nums[i]) dq.pop_back();  // dominated
        dq.push_back(i);
        if (i + 1 >= k) out.push_back(nums[dq.front()]);
    }
    return out;
}

int main() {
    assert((max_sliding_window({1, 3, -1, -3, 5, 3, 6, 7}, 3)
            == std::vector<int>{3, 3, 5, 5, 6, 7}));
    assert((max_sliding_window({1}, 1) == std::vector<int>{1}));
    assert((max_sliding_window({1, -1}, 1) == std::vector<int>{1, -1}));
    assert((max_sliding_window({9, 11}, 2) == std::vector<int>{11}));
    assert((max_sliding_window({4, 3, 2, 1}, 2) == std::vector<int>{4, 3, 2}));  // decreasing
    assert((max_sliding_window({1, 2, 3, 4}, 2) == std::vector<int>{2, 3, 4}));  // increasing
    assert((max_sliding_window({7, 7, 7}, 2) == std::vector<int>{7, 7}));        // equal
    assert(max_sliding_window({}, 3).empty());
    assert(max_sliding_window({1, 2}, 5).empty());   // k larger than the input
    return 0;
}
