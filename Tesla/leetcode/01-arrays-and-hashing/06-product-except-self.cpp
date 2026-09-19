// PROBLEM     Return an array where out[i] is the product of every element EXCEPT nums[i].
//             No division allowed; O(n) time.
// EXAMPLE     [1,2,3,4] -> [24,12,8,6]
// APPROACH    out[i] = (product of everything left of i) * (product of everything right).
//             Two passes: a forward pass writing the prefix product into out, then a
//             backward pass multiplying by a running suffix product held in a scalar.
//             That gives O(1) extra space (the output does not count).
// COMPLEXITY  Time O(n), Space O(1) extra. The division approach is O(n) too but breaks
//             on a zero -- and "no division" is usually the actual point of the question.
// FOLLOW-UPS  Why no division? -> zeros, and integer overflow/precision on floats.
//             Overflow? -> the product of n 32-bit ints overflows fast; use long long or
//             say the constraint. Many queries on a static array? -> precompute both
//             prefix and suffix arrays (O(n) space, O(1) per query).
//             Zeros? -> this solution handles any number of zeros correctly with no
//             special-casing, which is worth pointing out.

#include <cassert>
#include <vector>

std::vector<long long> product_except_self(const std::vector<int>& nums) {
    const std::size_t n = nums.size();
    std::vector<long long> out(n, 1);               // long long: the product overflows int
    if (n == 0) return out;

    // Pass 1: out[i] = product of nums[0..i-1]
    long long prefix = 1;
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = prefix;
        prefix *= nums[i];
    }
    // Pass 2: multiply in the product of nums[i+1..n-1], carried in a scalar.
    long long suffix = 1;
    for (std::size_t i = n; i-- > 0;) {
        out[i] *= suffix;
        suffix *= nums[i];
    }
    return out;
}

int main() {
    assert((product_except_self({1, 2, 3, 4}) == std::vector<long long>{24, 12, 8, 6}));
    assert((product_except_self({-1, 1, 0, -3, 3}) == std::vector<long long>{0, 0, 9, 0, 0}));
    assert((product_except_self({0, 0}) == std::vector<long long>{0, 0}));   // two zeros
    assert((product_except_self({5, 0}) == std::vector<long long>{0, 5}));   // one zero
    assert((product_except_self({7}) == std::vector<long long>{1}));         // single
    assert(product_except_self({}).empty());
    assert((product_except_self({2, 3}) == std::vector<long long>{3, 2}));
    return 0;
}
