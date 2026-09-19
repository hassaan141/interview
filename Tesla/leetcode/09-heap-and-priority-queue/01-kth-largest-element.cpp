// PROBLEM     Find the kth largest element in an unsorted array.
// EXAMPLE     [3,2,1,5,6,4], k = 2 -> 5
// APPROACH    Three answers, and knowing WHICH to use is the interview:
//               (a) sort                  O(n log n), trivial
//               (b) size-k MIN-heap       O(n log k) time, O(k) SPACE -- the streaming answer
//               (c) std::nth_element       O(n) AVERAGE -- the right answer when the whole
//                                          array is in memory
// COMPLEXITY  (c) O(n) average, O(1) extra. Quickselect's worst case is O(n^2), which
//             libstdc++ avoids by falling back to heapselect (introselect).
// FOLLOW-UPS  Which do you pick? -> nth_element if the data fits and you may reorder it;
//             the bounded heap if it is a stream or k << n and you cannot hold n.
//             Why a MIN-heap for the LARGEST? -> so the smallest of your current best k is
//             at the top and can be evicted in O(log k).
//             kth SMALLEST? -> flip the comparator, or nth_element with k-1.
//             Repeated queries? -> sort once, then O(1) per query.

#include <algorithm>
#include <cassert>
#include <optional>
#include <queue>
#include <vector>

// (b) Bounded min-heap: O(n log k) time, O(k) space. Works on a stream.
std::optional<int> kth_largest_heap(const std::vector<int>& nums, std::size_t k) {
    if (k == 0 || k > nums.size()) return std::nullopt;
    std::priority_queue<int, std::vector<int>, std::greater<>> min_heap;
    for (int n : nums) {
        min_heap.push(n);
        if (min_heap.size() > k) min_heap.pop();      // evict the smallest of the best k
    }
    return min_heap.top();                            // the kth largest
}

// (c) nth_element: O(n) average. Mutates, so take by value and say so.
std::optional<int> kth_largest_select(std::vector<int> nums, std::size_t k) {
    if (k == 0 || k > nums.size()) return std::nullopt;
    const auto nth = nums.begin() + static_cast<std::ptrdiff_t>(k - 1);
    std::nth_element(nums.begin(), nth, nums.end(), std::greater<>{});
    return *nth;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{3, 2, 1, 5, 6, 4}, std::size_t{2}) == 5);
        assert(fn(std::vector<int>{3, 2, 3, 1, 2, 4, 5, 5, 6}, std::size_t{4}) == 4);
        assert(fn(std::vector<int>{1}, std::size_t{1}) == 1);
        assert(fn(std::vector<int>{2, 1}, std::size_t{2}) == 1);        // the smallest
        assert(fn(std::vector<int>{5, 5, 5}, std::size_t{2}) == 5);     // duplicates
        assert(!fn(std::vector<int>{1, 2}, std::size_t{3}).has_value());
        assert(!fn(std::vector<int>{1, 2}, std::size_t{0}).has_value());
        assert(!fn(std::vector<int>{}, std::size_t{1}).has_value());
        assert(fn(std::vector<int>{-1, -2, -3}, std::size_t{1}) == -1);  // negatives
    };
    check([](const std::vector<int>& v, std::size_t k) { return kth_largest_heap(v, k); });
    check([](std::vector<int> v, std::size_t k) { return kth_largest_select(std::move(v), k); });
    return 0;
}
