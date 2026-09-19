// PROBLEM     (I)   Every element appears twice except one. Find it.
//             (II)  Every element appears three times except one. Find it.
//             (III) Exactly two elements appear once, the rest twice. Find both.
// APPROACH    (I) XOR the whole array. a^a == 0 and a^0 == a, and XOR is commutative and
//             associative, so every pair cancels and the loner survives. O(n) time,
//             O(1) space -- the whole point, since a hash map would be O(n) space.
//             (II) XOR cannot work (three copies leave one behind). Instead count each bit
//             position modulo 3: a bit set in the answer appears 3k+1 times. The elegant
//             version uses two accumulators `ones` and `twos` as a mod-3 state machine.
//             (III) XOR everything to get a^b. Any set bit of that is a position where a
//             and b DIFFER, so partition the array by that bit and XOR each half
//             separately. Use the lowest set bit (x & -x) to pick it.
// COMPLEXITY  All O(n) time, O(1) space.
// FOLLOW-UPS  Why not a hash map? -> it works and is O(n) space; the constraint is the
//             question. On an embedded target the O(1)-space version may be the only
//             option.
//             Generalize to "every element appears k times"? -> count bits mod k.
//             Overflow/signedness? -> do the bit work in unsigned to avoid UB on
//             negation and shifts.

#include <cassert>
#include <cstdint>
#include <vector>

int single_number(const std::vector<int>& nums) {
    int result = 0;
    for (int n : nums) result ^= n;                  // pairs cancel
    return result;
}

// (II) Every element appears three times except one. A mod-3 bit counter.
int single_number_ii(const std::vector<int>& nums) {
    std::uint32_t ones = 0, twos = 0;
    for (int n : nums) {
        const auto v = static_cast<std::uint32_t>(n);
        ones = (ones ^ v) & ~twos;                   // a bit enters `ones` on count 1
        twos = (twos ^ v) & ~ones;                   // and `twos` on count 2; count 3 clears
    }
    return static_cast<int>(ones);
}

// (III) Two elements appear once. Split by a differing bit.
std::pair<int, int> single_number_iii(const std::vector<int>& nums) {
    std::uint32_t xor_all = 0;
    for (int n : nums) xor_all ^= static_cast<std::uint32_t>(n);

    const std::uint32_t lowest_diff = xor_all & (~xor_all + 1);   // x & -x, without UB
    std::uint32_t a = 0, b = 0;
    for (int n : nums) {
        const auto v = static_cast<std::uint32_t>(n);
        if (v & lowest_diff) a ^= v;                 // the two loners land in different
        else                 b ^= v;                 // buckets, and every pair cancels
    }
    return {static_cast<int>(a), static_cast<int>(b)};
}

int main() {
    assert(single_number({2, 2, 1}) == 1);
    assert(single_number({4, 1, 2, 1, 2}) == 4);
    assert(single_number({1}) == 1);
    assert(single_number({-1, -1, 5}) == 5);         // negatives
    assert(single_number({}) == 0);

    assert(single_number_ii({2, 2, 3, 2}) == 3);
    assert(single_number_ii({0, 1, 0, 1, 0, 1, 99}) == 99);
    assert(single_number_ii({-2, -2, 1, -2}) == 1);
    assert(single_number_ii({7}) == 7);

    const auto [x, y] = single_number_iii({1, 2, 1, 3, 2, 5});
    assert((x == 3 && y == 5) || (x == 5 && y == 3));
    const auto [p, q] = single_number_iii({-1, 0});
    assert((p == -1 && q == 0) || (p == 0 && q == -1));
    return 0;
}
