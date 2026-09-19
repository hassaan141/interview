// PROBLEM     Return true if any value appears at least twice.
// EXAMPLE     [1,2,3,1] -> true ;  [1,2,3,4] -> false
// APPROACH    Insert into a set; if an insert fails, it was already there. Early-exit on
//             the first duplicate, so the average case is much better than the worst.
// COMPLEXITY  Time O(n) average, Space O(n). The sort alternative is O(n log n) time and
//             O(1) extra space -- and often FASTER in practice for small n because it is
//             contiguous and cache friendly. Say both and name the trade.
// FOLLOW-UPS  Memory constrained? -> sort in place, or a Bloom filter for an approximate
//             answer. Streaming with no bound on n? -> a Bloom filter / count-min sketch.
//             Values in a known small range? -> a bitset, 1 bit per value.
//             "Duplicate within k indices"? -> a sliding-window set (section 03).

#include <algorithm>
#include <bitset>
#include <cassert>
#include <unordered_set>
#include <vector>

// The idiomatic answer.
bool contains_duplicate(const std::vector<int>& nums) {
    std::unordered_set<int> seen;
    seen.reserve(nums.size());
    for (int n : nums)
        if (!seen.insert(n).second) return true;    // insert returns {iter, inserted}
    return false;
}

// O(1) extra space, O(n log n) time. Mutates a copy, so state that.
bool contains_duplicate_sorted(std::vector<int> nums) {
    std::ranges::sort(nums);
    return std::ranges::adjacent_find(nums) != nums.end();
}

// When the value range is small and known: 1 bit per value, no allocation, L1-resident.
bool contains_duplicate_bitset(const std::vector<int>& nums) {
    std::bitset<1024> seen;                         // assumes 0 <= v < 1024
    for (int n : nums) {
        assert(n >= 0 && n < 1024 && "bitset version requires a bounded range");
        if (seen.test(static_cast<std::size_t>(n))) return true;
        seen.set(static_cast<std::size_t>(n));
    }
    return false;
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{1, 2, 3, 1}));
        assert(!fn(std::vector<int>{1, 2, 3, 4}));
        assert(fn(std::vector<int>{1, 1, 1, 3, 3, 4, 3, 2, 4, 2}));
        assert(!fn(std::vector<int>{}));
        assert(!fn(std::vector<int>{7}));
    };
    check([](const std::vector<int>& v) { return contains_duplicate(v); });
    check([](const std::vector<int>& v) { return contains_duplicate_bitset(v); });
    assert(contains_duplicate_sorted({1, 2, 3, 1}));
    assert(!contains_duplicate_sorted({1, 2, 3, 4}));
    assert(contains_duplicate({-1, 0, -1}));       // negatives: the bitset cannot
    return 0;
}
