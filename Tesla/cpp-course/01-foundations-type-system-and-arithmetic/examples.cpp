// 01 — Foundations: type system, integral and floating-point arithmetic.
//
//   g++ -std=c++20 -Wall -Wextra -Wconversion -g examples.cpp -o ex && ./ex
//
// Every assert here must pass. Break them on purpose to learn.

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------- type traits
struct Pod        { int a; double b; };            // trivial + standard layout
struct WithVtable { virtual ~WithVtable() = default; int a; };
struct MixedAccess{ public: int a; private: int b; };

static void type_properties() {
    static_assert(std::is_trivially_copyable_v<Pod>);
    static_assert(std::is_standard_layout_v<Pod>);
    static_assert(std::is_aggregate_v<Pod>);

    // A polymorphic class is neither: it has a vptr the compiler owns.
    static_assert(!std::is_trivially_copyable_v<WithVtable>);
    static_assert(!std::is_standard_layout_v<WithVtable>);

    // Mixed access control breaks standard layout (order between the two
    // access groups is unspecified).
    static_assert(!std::is_standard_layout_v<MixedAccess>);

    // This is the guarantee that lets you memcpy a frame into shared memory.
    static_assert(sizeof(Pod) == 16);      // 4 + 4 padding + 8
    static_assert(alignof(Pod) == 8);
}

// ------------------------------------------------------------ auto deduction
static void auto_decays() {
    const std::vector<int> v{1, 2, 3};

    auto  a = v;                                       // copy, const dropped
    auto& r = v;                                       // const vector<int>&
    static_assert(!std::is_const_v<decltype(a)>);
    static_assert(std::is_const_v<std::remove_reference_t<decltype(r)>>);
    a.push_back(4);                                    // legal: a is a mutable copy
    assert(a.size() == 4 && v.size() == 3);

    int arr[5]{};
    auto  p = arr;                                     // int*  (decayed)
    auto& q = arr;                                     // int(&)[5]
    static_assert(std::is_same_v<decltype(p), int*>);
    static_assert(std::is_same_v<decltype(q), int(&)[5]>);
    static_assert(sizeof(q) == 5 * sizeof(int));
    (void)p;

    // The vector<bool> proxy trap.
    std::vector<bool> bits{true, false};
    auto proxy = bits[0];
    static_assert(!std::is_same_v<decltype(proxy), bool>);  // NOT bool!
    bool real = bits[0];
    assert(real);

    // map value_type is pair<const K, V> — the wrong loop type copies.
    std::map<std::string, int> m{{"a", 1}};
    for (const auto& [k, val] : m) { assert(k == "a" && val == 1); }
}

// ------------------------------------------------- integral promotion & signs
static void integer_conversions() {
    unsigned char a = 200, b = 200;
    int as_int = a + b;                 // both promoted to int: no wraparound
    assert(as_int == 400);
    auto truncated = static_cast<unsigned char>(a + b);
    assert(truncated == 144);           // 400 mod 256, well-defined but a bug

    int      x = -1;
    unsigned u = 1;
    // The classic: x is converted to unsigned, becoming UINT_MAX.
    // -Wsign-compare fires here on purpose -- the warning IS the lesson. In real
    // code you fix the comparison, you do not silence it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
    assert(!(x < u));
#pragma GCC diagnostic pop
    assert(std::cmp_less(x, u));        // C++20: compares mathematical values
    assert(!std::in_range<std::int8_t>(300));

    // The off-by-one that reads out of bounds.
    std::vector<int> empty;
    assert(empty.size() - 1 == std::numeric_limits<std::size_t>::max());
    // safe forms:
    for (std::size_t i = 0; i + 1 < empty.size(); ++i) { assert(false); }
    assert(std::ssize(empty) - 1 == -1);          // C++20 signed size

    // Unsigned wraparound is defined; signed overflow would be UB.
    std::uint8_t w = 255;
    ++w;
    assert(w == 0);

    // Checked arithmetic instead of relying on UB.
    int out = 0;
    assert(!__builtin_add_overflow(2, 3, &out) && out == 5);
    assert(__builtin_add_overflow(std::numeric_limits<int>::max(), 1, &out));
}

// ----------------------------------------------------------- floating point
static bool almost_equal(double lhs, double rhs,
                         double atol = 1e-12, double rtol = 1e-9) {
    const double diff = std::fabs(lhs - rhs);
    if (diff <= atol) return true;                       // handles near-zero
    return diff <= rtol * std::max(std::fabs(lhs), std::fabs(rhs));
}

static void floating_point() {
    // 0.1 is not 0.1.
    assert(0.1 + 0.2 != 0.3);
    assert(almost_equal(0.1 + 0.2, 0.3));

    // Addition is not associative -> parallel reductions differ from serial.
    const double left  = (0.1 + 0.2) + 0.3;
    const double right = 0.1 + (0.2 + 0.3);
    assert(left != right);
    assert(almost_equal(left, right));

    // NaN compares unequal to itself.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    assert(nan != nan);
    assert(std::isnan(nan));
    assert(!(nan < 1.0) && !(nan >= 1.0));     // every ordered compare is false

    // Signed zeros compare equal but are not the same value.
    assert(0.0 == -0.0);
    assert(std::signbit(-0.0) && !std::signbit(0.0));
    assert(std::isinf(1.0 / 0.0) && 1.0 / -0.0 < 0.0);

    // Catastrophic cancellation: the naive variance can go negative.
    const double data[] = {1e8, 1e8 + 1.0, 1e8 + 2.0};
    double sum = 0, sum_sq = 0;
    for (double d : data) { sum += d; sum_sq += d * d; }
    const double n        = 3.0;
    const double mean     = sum / n;
    const double naive    = sum_sq / n - mean * mean;      // digits annihilated
    double welford_m = 0, welford_s = 0, count = 0;        // stable
    for (double d : data) {
        ++count;
        const double delta = d - welford_m;
        welford_m += delta / count;
        welford_s += delta * (d - welford_m);
    }
    const double stable = welford_s / n;
    assert(almost_equal(stable, 2.0 / 3.0, 1e-9, 1e-6));
    std::cout << "  naive variance  = " << naive  << "  (should be 0.667)\n"
              << "  Welford variance= " << stable << '\n';

    // Machine epsilon is the gap at 1.0, not an absolute error bound.
    constexpr double eps = std::numeric_limits<double>::epsilon();
    assert(1.0 + eps != 1.0);
    assert(1.0 + eps / 2.0 == 1.0);
    assert(1e16 + 1.0 == 1e16);            // eps scales with magnitude
}

// ------------------------------------------------------- operators & ordering
struct Point {
    int x{}, y{};
    auto operator<=>(const Point&) const = default;
    bool operator==(const Point&) const = default;
};

static void operators() {
    int i = 5;
    const int post = i++;    // returns the old value (needs a copy for classes)
    const int pre  = ++i;    // returns the object itself
    assert(post == 5 && pre == 7 && i == 7);

    // Precedence: & binds LOOSER than ==. Parenthesize bit ops, always.
    constexpr int flags = 0b0110;
    static_assert((flags & 0b0010) == 0b0010);      // what you meant
    static_assert((flags & (0b0010 == 0b0010)) == 0);  // what you wrote unparenthesised

    static_assert(Point{1, 2} < Point{1, 3});       // lexicographic, from <=>
    static_assert(Point{1, 2} == Point{1, 2});
    // Floats give a PARTIAL ordering because of NaN:
    static_assert(std::is_same_v<decltype(1.0 <=> 2.0), std::partial_ordering>);
    static_assert(std::is_same_v<decltype(1 <=> 2), std::strong_ordering>);
}

int main() {
    type_properties();
    auto_decays();
    integer_conversions();
    std::cout << "floating point:\n";
    floating_point();
    operators();
    std::cout << "section 01: all checks passed\n";
}
