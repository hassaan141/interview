// PROBLEM     (a) Count the set bits in one integer. (b) Do it for every integer 0..n.
// APPROACH    (a) THREE answers, best first:
//               1. std::popcount(x)          -- C++20, compiles to a single POPCNT/CNT
//                                               instruction. This is the answer.
//               2. Brian Kernighan: x &= x - 1 clears the LOWEST set bit, so the loop runs
//                  once per SET bit, not once per bit. O(set bits).
//               3. Naive shift loop: O(bit width).
//             (b) DP: count[i] = count[i >> 1] + (i & 1). Dropping the low bit gives an
//             already-computed smaller number -- O(n) total instead of O(n log n).
//             An equivalent form is count[i] = count[i & (i-1)] + 1.
// COMPLEXITY  (a) O(1) with popcount, O(set bits) with Kernighan. (b) O(n) time and space.
// FOLLOW-UPS  Why does x & (x-1) clear the lowest set bit? -> subtracting 1 flips that bit
//             to 0 and everything below it to 1; the AND then clears the whole low run.
//             Hamming DISTANCE? -> popcount(a ^ b).
//             Parity only? -> a folding XOR, or the compiler builtin.
//             Real use: population count is how you size a bitset-backed free list, count
//             active lanes in a SIMD mask, or compute a sparse index.

#include <bit>
#include <cassert>
#include <cstdint>
#include <vector>

// Brian Kernighan: one iteration per SET bit.
int hamming_weight_kernighan(std::uint32_t x) {
    int count = 0;
    while (x) { x &= x - 1; ++count; }              // clears the lowest set bit
    return count;
}

// (b) DP over the low bit.
std::vector<int> counting_bits(int n) {
    if (n < 0) return {};
    std::vector<int> count(static_cast<std::size_t>(n) + 1, 0);
    for (int i = 1; i <= n; ++i)
        count[static_cast<std::size_t>(i)] =
            count[static_cast<std::size_t>(i >> 1)] + (i & 1);      // reuse i/2
    return count;
}

int hamming_distance(std::uint32_t a, std::uint32_t b) {
    return std::popcount(a ^ b);                     // differing bits
}

int main() {
    assert(std::popcount(0b1011u) == 3);
    assert(hamming_weight_kernighan(0b1011) == 3);
    assert(hamming_weight_kernighan(0) == 0);
    assert(hamming_weight_kernighan(0xFFFFFFFFu) == 32);
    assert(hamming_weight_kernighan(0x80000000u) == 1);

    // The library, Kernighan and the DP must all agree.
    for (std::uint32_t x = 0; x < 1000; ++x)
        assert(hamming_weight_kernighan(x) == std::popcount(x));

    const auto bits = counting_bits(5);
    assert((bits == std::vector<int>{0, 1, 1, 2, 1, 2}));
    assert(counting_bits(0) == std::vector<int>{0});
    assert(counting_bits(-1).empty());
    const auto big = counting_bits(255);
    assert(big[255] == 8 && big[128] == 1 && big[7] == 3);

    assert(hamming_distance(1, 4) == 2);             // 0001 vs 0100
    assert(hamming_distance(3, 3) == 0);
    return 0;
}
