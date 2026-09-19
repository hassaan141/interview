# 16 — Bit Manipulation and Math

## Why this section matters for *this* job

Bit tricks are everyday vocabulary in embedded and systems code: flags and register
masks, alignment, ring-buffer index wrapping, checksums, packed formats. An autonomy
foundations interviewer will expect you to be fluent, not clever.

## The vocabulary

```cpp
x & 1                    // is odd
x >> 1                   // divide by 2 (for UNSIGNED; arithmetic shift for signed)
x & (x - 1)              // clear the lowest set bit    -> Brian Kernighan's popcount
x & -x                   // ISOLATE the lowest set bit
x | (x + 1)              // set the lowest clear bit
x ^ y                    // 1 where they differ; x ^ x == 0; x ^ 0 == x
~x                       // complement
x & (x - 1) == 0         // is a power of two (plus x != 0)
x & (n - 1)              // x % n, when n is a power of two  -> ring buffers
(x + n - 1) & ~(n - 1)   // round x UP to a multiple of n    -> alignment
```

C++20 `<bit>` — **use these instead of hand-rolling**, they compile to single
instructions:
```cpp
std::popcount(x)        std::has_single_bit(x)     std::bit_ceil(x)  std::bit_floor(x)
std::countl_zero(x)     std::countr_zero(x)        std::bit_width(x)
std::rotl(x, n)         std::rotr(x, n)            std::byteswap(x)   // C++23
std::bit_cast<To>(x)    // the safe reinterpret (course section 03)
```

## XOR properties (the basis of half these problems)

`a ^ a == 0`, `a ^ 0 == a`, XOR is commutative and associative. So XOR-ing a whole array
cancels every value that appears an even number of times — which finds the one that does
not, in O(n) time and **O(1) space** with no hash map.

## Math worth having ready

- **Overflow**: signed overflow is UB (course section 01). `a + b` can overflow;
  `a + (b - a) / 2` and `std::midpoint` do not.
- **Modular arithmetic**: `(a + b) % m`, `(a * b) % m` with `long long`; fast
  exponentiation is O(log n).
- **GCD/LCM**: `std::gcd`, `std::lcm`. `lcm(a,b) = a / gcd(a,b) * b` — divide **first**
  to avoid overflow.
- **Primes**: sieve of Eratosthenes, O(n log log n).
- **Digits**: `n % 10` and `n /= 10`, and watch `INT_MIN` (its negation overflows).

## Problems

| File | Problem | Idea |
| --- | --- | --- |
| `01-single-number.cpp` | Single Number I / II / III | XOR cancellation |
| `02-counting-bits.cpp` | Number of 1 Bits / Counting Bits | Kernighan, and a DP |
| `03-reverse-bits.cpp` | Reverse Bits | divide and conquer, and byte tables |
| `04-missing-number.cpp` | Missing Number | XOR or Gauss sum |
| `05-sum-of-two-integers.cpp` | Sum of Two Integers | add without `+` (a half adder) |
| `06-power-and-sqrt.cpp` | Pow(x,n), Sqrt(x) | fast exponentiation, integer sqrt |
| `07-bit-utilities.cpp` | Alignment, masks, ring-buffer wrap | **the practical systems ones** |

## Traps

1. Shifting by >= the bit width, or by a negative amount: **UB**.
2. Left-shifting a negative signed value: UB before C++20.
3. `-x` on `INT_MIN`: overflow, UB. Use unsigned for bit work.
4. `x >> 1` on a negative `int` is implementation-defined-ish (arithmetic shift in
   practice, and well-defined since C++20). Use unsigned types for bit manipulation.
5. `x % n` with a power-of-two `n` and a **signed** `x` is *not* `x & (n-1)` for negatives.
6. Assuming `int` is 32 bits — use `<cstdint>` (course section 01).
