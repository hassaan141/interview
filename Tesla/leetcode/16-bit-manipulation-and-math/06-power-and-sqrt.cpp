// PROBLEM     (a) x^n for a real x and an integer n. (b) Integer square root of x.
// APPROACH    (a) FAST EXPONENTIATION (exponentiation by squaring): x^n = (x^(n/2))^2 for
//             even n, and x * x^(n-1) for odd. O(log n) multiplications instead of O(n).
//             THE TRAP: n can be INT_MIN, and -INT_MIN overflows. Widen to long long
//             BEFORE negating -- this is the specific bug the problem is testing.
//             (b) BINARY SEARCH on the answer (section 05): find the largest m with
//             m*m <= x. Compare with m <= x / m rather than m * m <= x to avoid overflow.
//             Newton's method converges faster (quadratically) and is worth naming.
// COMPLEXITY  (a) O(log n). (b) O(log x) either way.
// FOLLOW-UPS  Modular exponentiation? -> the same loop with a % after each multiply; the
//             basis of RSA and of hashing.
//             Floating point accuracy? -> repeated squaring accumulates rounding error;
//             std::pow is more accurate for real exponents, and this version is for
//             INTEGER exponents. Say so (course section 01).
//             Fast inverse square root? -> the famous Quake trick is a bit_cast plus a
//             magic constant plus one Newton step; today `rsqrtss`/`FRSQRTE` is better.

#include <cassert>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>

// (a) Exponentiation by squaring.
double my_pow(double x, int n) {
    // Widen BEFORE negating: -INT_MIN is UB.
    long long exponent = n;
    if (exponent < 0) { x = 1.0 / x; exponent = -exponent; }

    double result = 1.0;
    while (exponent > 0) {
        if (exponent & 1) result *= x;               // an odd bit: fold x in
        x *= x;                                       // square for the next bit
        exponent >>= 1;
    }
    return result;
}

// Modular exponentiation: the same loop, the basis of RSA.
std::uint64_t pow_mod(std::uint64_t base, std::uint64_t exponent, std::uint64_t modulus) {
    std::uint64_t result = 1;
    base %= modulus;
    while (exponent > 0) {
        if (exponent & 1) result = (result * base) % modulus;
        base = (base * base) % modulus;
        exponent >>= 1;
    }
    return result;
}

// (b) Integer square root by binary search. Note the division to avoid overflow.
int my_sqrt(int x) {
    if (x < 0) return -1;
    if (x < 2) return x;
    int lo = 1, hi = x / 2 + 1;                       // sqrt(x) <= x/2 for x >= 4
    while (lo < hi) {
        const int mid = lo + (hi - lo + 1) / 2;       // round UP: we search the last true
        if (mid <= x / mid) lo = mid;                 // mid*mid <= x, without overflow
        else                hi = mid - 1;
    }
    return lo;
}

// Newton's method: quadratic convergence, fewer iterations.
int my_sqrt_newton(int x) {
    if (x < 2) return x < 0 ? -1 : x;
    std::int64_t guess = x;
    while (guess * guess > x) guess = (guess + x / guess) / 2;
    return static_cast<int>(guess);
}

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

int main() {
    assert(near(my_pow(2.0, 10), 1024.0));
    assert(near(my_pow(2.1, 3), 9.261));
    assert(near(my_pow(2.0, -2), 0.25));
    assert(near(my_pow(5.0, 0), 1.0));
    assert(near(my_pow(1.0, 2147483647), 1.0));
    // INT_MIN: the case that breaks a naive `n = -n`.
    assert(near(my_pow(1.0, std::numeric_limits<int>::min()), 1.0));
    assert(my_pow(2.0, std::numeric_limits<int>::min()) < 1e-300);

    assert(pow_mod(2, 10, 1000) == 24);              // 1024 mod 1000
    assert(pow_mod(3, 0, 7) == 1);
    assert(pow_mod(2, 62, 1000000007) == 145586002);

    for (int x : {0, 1, 2, 3, 4, 8, 9, 15, 16, 17, 2147395599, 2147483647}) {
        assert(my_sqrt(x) == static_cast<int>(std::sqrt(static_cast<double>(x))));
        assert(my_sqrt_newton(x) == my_sqrt(x));
    }
    assert(my_sqrt(-1) == -1);
    return 0;
}
