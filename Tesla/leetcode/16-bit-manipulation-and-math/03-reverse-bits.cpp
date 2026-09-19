// PROBLEM     Reverse the bits of a 32-bit unsigned integer.
// APPROACH    Three levels, and the interviewer wants the second or third:
//             1. Loop 32 times shifting bits across. O(32), correct, dull.
//             2. DIVIDE AND CONQUER: swap 16-bit halves, then 8-bit groups, then 4, 2, 1.
//                Five masked shift-or steps, no loop, O(log 32) operations. This is the
//                classic and it generalizes to any width.
//             3. A 256-entry BYTE LOOKUP TABLE: reverse four bytes and reassemble. Four
//                loads plus shifts -- the fastest portable version, and the one a real
//                codebase uses (with the table generated `constexpr` so it costs nothing
//                at startup).
//             In production you would first check for a hardware instruction: ARM has RBIT
//             (`__builtin_arm_rbit`), x86 does not. Saying that is the systems answer.
// COMPLEXITY  All O(1); the difference is the constant factor.
// FOLLOW-UPS  Reverse only the low k bits? -> reverse 32 then shift right by 32-k.
//             Byte order (endianness) is a DIFFERENT operation -- std::byteswap (C++23) or
//             __builtin_bswap32. Confusing bit reversal with byte swapping is a common
//             and revealing mistake.
//             Called in a loop? -> the table version, and mention that the table is 256
//             bytes, so it stays in L1.

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>

// 1. The obvious loop.
std::uint32_t reverse_bits_loop(std::uint32_t x) {
    std::uint32_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result = (result << 1) | (x & 1u);
        x >>= 1;
    }
    return result;
}

// 2. Divide and conquer: swap halves, then quarters, and so on.
constexpr std::uint32_t reverse_bits_swap(std::uint32_t x) {
    x = (x >> 16) | (x << 16);                                       // 16-bit halves
    x = ((x & 0xFF00FF00u) >> 8) | ((x & 0x00FF00FFu) << 8);         // bytes
    x = ((x & 0xF0F0F0F0u) >> 4) | ((x & 0x0F0F0F0Fu) << 4);         // nibbles
    x = ((x & 0xCCCCCCCCu) >> 2) | ((x & 0x33333333u) << 2);         // pairs
    x = ((x & 0xAAAAAAAAu) >> 1) | ((x & 0x55555555u) << 1);         // bits
    return x;
}

// 3. Byte table, generated at compile time so it costs nothing at startup.
constexpr std::array<std::uint8_t, 256> make_byte_table() {
    std::array<std::uint8_t, 256> table{};
    for (std::size_t i = 0; i < 256; ++i) {
        std::uint8_t v = 0;
        for (int b = 0; b < 8; ++b)
            if (i & (std::size_t{1} << b)) v = static_cast<std::uint8_t>(v | (1u << (7 - b)));
        table[i] = v;
    }
    return table;
}
constexpr auto kByteTable = make_byte_table();

std::uint32_t reverse_bits_table(std::uint32_t x) {
    return (static_cast<std::uint32_t>(kByteTable[x & 0xFF]) << 24) |
           (static_cast<std::uint32_t>(kByteTable[(x >> 8) & 0xFF]) << 16) |
           (static_cast<std::uint32_t>(kByteTable[(x >> 16) & 0xFF]) << 8) |
           (static_cast<std::uint32_t>(kByteTable[(x >> 24) & 0xFF]));
}

int main() {
    constexpr std::uint32_t input = 0b00000010100101000001111010011100u;
    constexpr std::uint32_t expected = 0b00111001011110000010100101000000u;
    static_assert(reverse_bits_swap(input) == expected);   // verified at COMPILE time

    assert(reverse_bits_loop(input) == expected);
    assert(reverse_bits_table(input) == expected);

    // All three must agree, including at the boundaries.
    for (std::uint32_t x : {0u, 1u, 0xFFFFFFFFu, 0x80000000u, 0xDEADBEEFu, 12345u}) {
        assert(reverse_bits_loop(x) == reverse_bits_swap(x));
        assert(reverse_bits_swap(x) == reverse_bits_table(x));
        assert(reverse_bits_swap(reverse_bits_swap(x)) == x);   // an involution
    }

    // Bit reversal is NOT byte swapping -- a distinction worth demonstrating.
    assert(std::rotl(0x80000001u, 1) == 0x00000003u);
    assert(reverse_bits_swap(0x000000FFu) == 0xFF000000u);
    return 0;
}
