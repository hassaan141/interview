// PROBLEM     An array holds n distinct numbers from the range [0, n]. Find the missing one.
// EXAMPLE     [3,0,1] -> 2
// APPROACH    Three O(n) answers; the differences are what matters:
//             (a) XOR: xor together every index 0..n and every value. Each present number
//                 appears exactly twice and cancels; the missing one survives. O(1) space,
//                 and CANNOT OVERFLOW -- that is its advantage.
//             (b) Gauss sum: n*(n+1)/2 minus the actual sum. Simplest to say, but the sum
//                 can overflow int for large n, so use long long.
//             (c) Sort or a hash set: O(n log n) or O(n) space -- mention and reject.
// COMPLEXITY  (a) and (b): O(n) time, O(1) space.
// FOLLOW-UPS  Which do you ship? -> XOR, because it has no overflow failure mode. That
//             reasoning is the answer they want.
//             TWO numbers missing? -> XOR gives a^b; split by a differing bit (see
//             problem 01 part III).
//             Find the DUPLICATE in [1,n] with n+1 elements (LC 287)? -> Floyd's cycle
//             detection treating the array as a function, O(n)/O(1) -- the same algorithm
//             as linked-list cycle detection (section 06).
//             Cannot modify the array and need O(1) space? -> that is exactly why the XOR
//             and Floyd answers exist.

#include <cassert>
#include <cstdint>
#include <numeric>
#include <vector>

// (a) XOR: no overflow, O(1) space.
int missing_number_xor(const std::vector<int>& nums) {
    std::uint32_t result = static_cast<std::uint32_t>(nums.size());   // start with n
    for (std::size_t i = 0; i < nums.size(); ++i)
        result ^= static_cast<std::uint32_t>(i) ^ static_cast<std::uint32_t>(nums[i]);
    return static_cast<int>(result);
}

// (b) Gauss sum: simpler to explain, but watch the overflow.
int missing_number_sum(const std::vector<int>& nums) {
    const long long n = static_cast<long long>(nums.size());
    const long long expected = n * (n + 1) / 2;                       // long long, not int
    const long long actual = std::accumulate(nums.begin(), nums.end(), 0LL);
    return static_cast<int>(expected - actual);
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<int>{3, 0, 1}) == 2);
        assert(fn(std::vector<int>{0, 1}) == 2);                      // n is missing
        assert(fn(std::vector<int>{1}) == 0);                         // 0 is missing
        assert(fn(std::vector<int>{0}) == 1);
        assert(fn(std::vector<int>{}) == 0);
        assert(fn(std::vector<int>{9, 6, 4, 2, 3, 5, 7, 0, 1}) == 8);
    };
    check([](const std::vector<int>& v) { return missing_number_xor(v); });
    check([](const std::vector<int>& v) { return missing_number_sum(v); });

    // A large input where the Gauss sum would overflow a 32-bit accumulator.
    std::vector<int> big(70000);
    std::iota(big.begin(), big.end(), 0);
    big.erase(big.begin() + 12345);                                   // remove one value
    assert(missing_number_xor(big) == 12345);
    assert(missing_number_sum(big) == 12345);                         // OK in long long
    return 0;
}
