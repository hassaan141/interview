// PROBLEM     Add two integers without using + or -.
// APPROACH    A HALF ADDER, iterated:
//               sum without carry  = a ^ b
//               carry              = (a & b) << 1
//             Repeat until the carry is zero. That is literally how a ripple-carry adder
//             works in hardware, which is the reason this question exists -- it checks
//             whether you can reason about the gate-level meaning of XOR and AND.
//             C++ DETAIL THAT MATTERS: do the arithmetic in UNSIGNED. Signed overflow and
//             left-shifting a negative value are undefined behavior (course section 01),
//             and negative inputs will absolutely trigger both.
// COMPLEXITY  O(bit width) iterations, O(1) space.
// FOLLOW-UPS  Subtraction? -> a + (~b + 1), i.e. add the two's complement.
//             MULTIPLICATION without *? -> shift-and-add (Russian peasant), below.
//             Why does the loop terminate? -> each iteration moves the carry at least one
//             bit left, so after at most 32 rounds it shifts out.
//             Real use: you would never write this, but you WILL read it in fixed-point
//             and DSP code, and the UB point is a genuine production concern.

#include <cassert>
#include <cstdint>
#include <limits>

int get_sum(int a, int b) {
    // Unsigned throughout: wrapping is defined, shifting is defined.
    auto x = static_cast<std::uint32_t>(a);
    auto y = static_cast<std::uint32_t>(b);
    while (y != 0) {
        const std::uint32_t carry = (x & y) << 1;    // where both bits are 1
        x = x ^ y;                                    // sum without the carry
        y = carry;
    }
    return static_cast<int>(x);
}

int get_difference(int a, int b) {
    return get_sum(a, get_sum(~b, 1));               // a + (-b), two's complement
}

// Multiplication by shift-and-add, for the same reason.
int multiply(int a, int b) {
    const bool negative = (a < 0) != (b < 0);
    auto x = static_cast<std::uint32_t>(a < 0 ? -static_cast<std::int64_t>(a) : a);
    auto y = static_cast<std::uint32_t>(b < 0 ? -static_cast<std::int64_t>(b) : b);
    std::uint32_t result = 0;
    while (y) {
        if (y & 1) result += x;                       // add the shifted multiplicand
        x <<= 1;
        y >>= 1;
    }
    const auto value = static_cast<std::int64_t>(result);
    return static_cast<int>(negative ? -value : value);
}

int main() {
    assert(get_sum(1, 2) == 3);
    assert(get_sum(2, 3) == 5);
    assert(get_sum(0, 0) == 0);
    assert(get_sum(-1, 1) == 0);
    assert(get_sum(-2, -3) == -5);                    // both negative
    assert(get_sum(-5, 3) == -2);                     // mixed signs
    assert(get_sum(5, -3) == 2);
    assert(get_sum(std::numeric_limits<int>::max(), 0) == std::numeric_limits<int>::max());

    assert(get_difference(5, 3) == 2);
    assert(get_difference(3, 5) == -2);
    assert(get_difference(0, -7) == 7);

    assert(multiply(3, 4) == 12);
    assert(multiply(-3, 4) == -12);
    assert(multiply(-3, -4) == 12);
    assert(multiply(0, 99) == 0);
    assert(multiply(1, -1) == -1);
    return 0;
}
