// 12 — STL: views, containers, iterators, algorithms, ranges, utilities.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -O2 -g examples.cpp -o ex && ./ex

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

// ------------------------------------------------------------------- views
static int sum_span(std::span<const int> s) {
    return std::accumulate(s.begin(), s.end(), 0);
}
static std::size_t count_char(std::string_view sv, char c) {
    return static_cast<std::size_t>(std::ranges::count(sv, c));
}

static void views() {
    // A span is two words and accepts anything contiguous.
    static_assert(sizeof(std::span<const int>) == 2 * sizeof(void*));
    static_assert(std::is_trivially_copyable_v<std::span<const int>>);

    std::vector<int> v{1, 2, 3, 4, 5};
    std::array<int, 3> a{10, 20, 30};
    int raw[]{7, 7};
    assert(sum_span(v) == 15 && sum_span(a) == 60 && sum_span(raw) == 14);

    // Subviews are free: no copy, no allocation.
    std::span<const int> s{v};
    assert(sum_span(s.subspan(1, 3)) == 9);
    assert(s.first(2).size() == 2 && s.last(2)[0] == 4);

    // A static extent puts the size in the TYPE.
    std::span<int, 3> fixed{a};
    static_assert(fixed.extent == 3);
    static_assert(sizeof(fixed) == sizeof(void*));   // no runtime size needed!

    assert(count_char("hello world", 'l') == 3);     // no allocation
    std::string_view sv{"prefix:value"};
    assert(sv.starts_with("prefix"));
    assert(sv.substr(7) == "value");
    // WARNING: sv.data() is NOT null-terminated after substr -- never hand it to a
    // C API. And never store a view that can outlive its buffer.
}

// ------------------------------------------------------- container complexity
static void containers() {
    // vector: contiguous, amortized O(1) push_back, invalidates on realloc.
    std::vector<int> v;
    v.reserve(4);
    const int* before = v.data();
    v.push_back(1); v.push_back(2); v.push_back(3); v.push_back(4);
    assert(v.data() == before);              // reserve() prevented reallocation
    v.push_back(5);                           // now it MUST grow
    assert(v.capacity() >= 5);
    // Every pointer/reference/iterator obtained before that push is now dangling.

    // deque: O(1) at both ends; references survive END insertion.
    std::deque<int> d{2, 3};
    d.push_front(1); d.push_back(4);
    int& front_ref = d.front();
    d.push_back(99);
    assert(front_ref == 1);                   // references survive push_back on deque
    assert(d.size() == 5);

    // map: ordered, O(log n). operator[] INSERTS -- that is the classic trap.
    std::map<std::string, int> m{{"b", 2}, {"a", 1}};
    assert(m.begin()->first == "a");          // sorted iteration
    assert(m["missing"] == 0 && m.size() == 3);   // <- inserted a zero!
    assert(m.count("missing") == 1);
    m.erase("missing");
    assert(m.find("nope") == m.end());        // the non-mutating lookup
    try { (void)m.at("nope"); assert(false); } catch (const std::out_of_range&) {}
    // C++17: try_emplace does not construct the value if the key exists;
    // insert_or_assign is the "upsert".
    auto [it, inserted] = m.try_emplace("a", 999);
    assert(!inserted && it->second == 1);     // untouched

    // unordered_map: O(1) average. Rehash invalidates ITERATORS, not REFERENCES.
    std::unordered_map<int, std::string> um;
    um.reserve(2);
    um.emplace(1, "one");
    std::string& ref = um[1];
    for (int i = 2; i < 100; ++i) um.emplace(i, "x");   // forces several rehashes
    assert(ref == "one");                     // the NODE never moved

    // set: ordered unique; lower_bound gives you range queries.
    std::set<int> s{1, 3, 5, 7, 9};
    assert(*s.lower_bound(4) == 5);           // first >= 4
    assert(*s.upper_bound(5) == 7);           // first > 5

    // priority_queue is a MAX-heap by default.
    std::priority_queue<int> max_heap;
    for (int x : {3, 1, 4, 1, 5}) max_heap.push(x);
    assert(max_heap.top() == 5);
    std::priority_queue<int, std::vector<int>, std::greater<>> min_heap;
    for (int x : {3, 1, 4, 1, 5}) min_heap.push(x);
    assert(min_heap.top() == 1);
}

// vector vs. list: the cache argument, measured.
static void vector_beats_list() {
    constexpr int kN = 20'000;
    std::vector<int> v(kN);
    std::list<int>   l(kN);
    std::iota(v.begin(), v.end(), 0);
    std::iota(l.begin(), l.end(), 0);

    auto time_it = [](auto&& fn) {
        const auto t0 = std::chrono::steady_clock::now();
        const long long r = fn();
        const auto t1 = std::chrono::steady_clock::now();
        return std::pair{std::chrono::duration<double, std::micro>(t1 - t0).count(), r};
    };

    const auto [v_us, v_sum] = time_it([&] {
        long long acc = 0;
        for (int i = 0; i < 20; ++i) for (int x : v) acc += x;
        return acc;
    });
    const auto [l_us, l_sum] = time_it([&] {
        long long acc = 0;
        for (int i = 0; i < 20; ++i) for (int x : l) acc += x;
        return acc;
    });
    assert(v_sum == l_sum);
    std::cout << "  traversal: vector " << v_us << " us vs list " << l_us
              << " us  (" << l_us / v_us << "x)\n";
    // Same algorithm, same element count. The list loses because each node is a
    // dependent pointer chase -- a potential cache miss -- while the vector walks
    // contiguous memory the prefetcher predicts perfectly.
}

// ---------------------------------------------------------- a custom iterator
// A strided view over a contiguous buffer: exactly what you need to walk one
// channel of an interleaved sensor buffer without copying.
class StrideIter {
    const int* p_{};
    std::ptrdiff_t stride_{1};
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = int;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const int*;
    using reference         = const int&;

    StrideIter() = default;
    StrideIter(const int* p, std::ptrdiff_t stride) : p_{p}, stride_{stride} {}

    reference operator*() const { return *p_; }
    reference operator[](difference_type n) const { return p_[n * stride_]; }

    StrideIter& operator++()    { p_ += stride_; return *this; }
    StrideIter  operator++(int) { auto t = *this; ++*this; return t; }
    StrideIter& operator--()    { p_ -= stride_; return *this; }
    StrideIter& operator+=(difference_type n) { p_ += n * stride_; return *this; }
    StrideIter& operator-=(difference_type n) { return *this += -n; }
    friend StrideIter operator+(StrideIter i, difference_type n) { return i += n; }
    friend StrideIter operator-(StrideIter i, difference_type n) { return i -= n; }
    friend difference_type operator-(const StrideIter& a, const StrideIter& b) {
        return (a.p_ - b.p_) / a.stride_;
    }
    friend bool operator==(const StrideIter&, const StrideIter&) = default;
    friend auto operator<=>(const StrideIter& a, const StrideIter& b) {
        return a.p_ <=> b.p_;
    }
};

static void custom_iterator() {
    // Interleaved [x,y,z, x,y,z, ...]; walk only the x channel.
    const int xyz[]{1, 10, 100,  2, 20, 200,  3, 30, 300};
    StrideIter xbegin{xyz, 3};
    StrideIter xend = xbegin + 3;                  // == xyz + 9, one-past-the-end: OK
    assert(std::distance(xbegin, xend) == 3);
    assert(std::accumulate(xbegin, xend, 0) == 6);
    assert(*std::max_element(xbegin, xend) == 3);
    assert(xbegin[2] == 3);                        // random access works

    // NOTE, and this is a real lesson, not a detail: a strided END POINTER for a
    // channel at a non-zero offset would be xyz + 1 + 3*3 == xyz + 10, which is
    // PAST one-past-the-end -- undefined behaviour merely to form, and gcc's
    // -Warray-bounds catches it. That is exactly why real strided ranges carry a
    // COUNT or a SENTINEL instead of an end pointer (std::counted_iterator,
    // std::views::stride in C++23, std::mdspan with layout_stride).
    auto y_channel = std::views::iota(std::size_t{0}, std::size_t{3})
                   | std::views::transform([&](std::size_t i) { return xyz[1 + 3 * i]; });
    assert(std::accumulate(y_channel.begin(), y_channel.end(), 0) == 60);
    static_assert(std::is_same_v<
        std::iterator_traits<StrideIter>::iterator_category,
        std::random_access_iterator_tag>);
}

// ------------------------------------------------------------------ algorithms
static void algorithms() {
    std::vector<int> v{5, 3, 8, 1, 9, 2, 7};

    // erase-remove: remove_if only SHUFFLES; it cannot resize the container.
    std::vector<int> a = v;
    const auto new_end = std::remove_if(a.begin(), a.end(), [](int x) { return x < 5; });
    assert(a.size() == 7);                          // size UNCHANGED
    a.erase(new_end, a.end());                      // <- the part people forget
    assert((a == std::vector<int>{5, 8, 9, 7}));
    std::vector<int> b = v;
    std::erase_if(b, [](int x) { return x < 5; });   // C++20: do this instead
    assert(a == b);

    // nth_element: O(n) average. THE answer for a median or top-k.
    std::vector<int> med = v;
    const auto mid = med.begin() + static_cast<std::ptrdiff_t>(med.size() / 2);
    std::nth_element(med.begin(), mid, med.end());
    assert(*mid == 5);                               // the median, without sorting
    std::vector<int> topk = v;
    std::partial_sort(topk.begin(), topk.begin() + 3, topk.end(), std::greater<>{});
    assert((std::vector<int>(topk.begin(), topk.begin() + 3) == std::vector<int>{9, 8, 7}));

    // binary search on a SORTED range.
    std::vector<int> sorted = v;
    std::ranges::sort(sorted);
    assert(std::binary_search(sorted.begin(), sorted.end(), 8));
    assert(*std::lower_bound(sorted.begin(), sorted.end(), 6) == 7);   // first >= 6
    assert(*std::upper_bound(sorted.begin(), sorted.end(), 7) == 8);   // first > 7

    // accumulate's INIT TYPE decides the accumulator type -- a classic silent bug.
    const std::vector<double> d{0.5, 0.5, 0.5};
    assert(std::accumulate(d.begin(), d.end(), 0) == 0);      // int accumulator!
    assert(std::accumulate(d.begin(), d.end(), 0.0) == 1.5);  // correct

    // transform_reduce: a fused dot product.
    const std::vector<double> x{1, 2, 3}, y{4, 5, 6};
    assert(std::transform_reduce(x.begin(), x.end(), y.begin(), 0.0,
                                 std::plus<>{}, std::multiplies<>{}) == 32.0);

    // back_inserter + copy_if, and the ostream iterator.
    std::vector<int> odds;
    std::copy_if(v.begin(), v.end(), std::back_inserter(odds),
                 [](int n) { return n % 2 != 0; });
    assert((odds == std::vector<int>{5, 3, 1, 9, 7}));

    // rotate, unique, clamp, iota, midpoint
    std::vector<int> r{1, 2, 3, 4, 5};
    std::rotate(r.begin(), r.begin() + 2, r.end());
    assert((r == std::vector<int>{3, 4, 5, 1, 2}));
    std::vector<int> dup{1, 1, 2, 2, 2, 3};
    dup.erase(std::unique(dup.begin(), dup.end()), dup.end());   // needs SORTED input
    assert((dup == std::vector<int>{1, 2, 3}));
    assert(std::clamp(15, 0, 10) == 10);
    assert(std::midpoint(2, 8) == 5);                // overflow-safe (a+b)/2
    static_assert(std::gcd(12, 18) == 6 && std::lcm(4, 6) == 12);
}

// ---------------------------------------------------------------- C++20 ranges
static void ranges_demo() {
    namespace rv = std::views;
    const std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Lazy: nothing is computed until the range is iterated, and there is no
    // intermediate container.
    auto pipeline = v | rv::filter([](int x) { return x % 2 == 0; })
                      | rv::transform([](int x) { return x * x; })
                      | rv::take(3);
    std::vector<int> out;
    for (int x : pipeline) out.push_back(x);
    assert((out == std::vector<int>{4, 16, 36}));

    // Range algorithms take the range directly, and support PROJECTIONS.
    struct Point { int id; double score; };
    std::vector<Point> pts{{1, 0.5}, {2, 0.9}, {3, 0.1}};
    const auto best = std::ranges::max_element(pts, {}, &Point::score);
    assert(best->id == 2);
    std::ranges::sort(pts, std::less<>{}, &Point::score);      // sort BY score
    assert(pts.front().id == 3);

    // iota / reverse / drop, and a keys view over a map.
    auto first_five = rv::iota(0, 100) | rv::take(5) | rv::reverse;
    assert((std::vector<int>(first_five.begin(), first_five.end())
            == std::vector<int>{4, 3, 2, 1, 0}));
    const std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    auto keys = m | rv::keys;
    assert((std::vector<std::string>(keys.begin(), keys.end())
            == std::vector<std::string>{"a", "b"}));

    // DANGLING-VIEW HAZARD: a view over a temporary container. `auto bad =
    // make_vector() | rv::filter(pred);` leaves the vector dead and the view
    // dangling. Materialize it, or keep the owner alive in a named variable.
}

// ---------------------------------------------------------------- utilities
static void utilities() {
    // optional: value stored INLINE, no allocation.
    static_assert(sizeof(std::optional<int>) == 8);          // int + bool + padding
    std::optional<int> o;
    assert(!o && o.value_or(-1) == -1);
    o = 5;
    assert(o && *o == 5);

    // variant: largest alternative inline + a tag. No allocation.
    std::variant<int, double, std::string> var = 3.5;
    assert(var.index() == 1 && std::holds_alternative<double>(var));
    assert(std::get_if<int>(&var) == nullptr);
    const std::string desc = std::visit([](const auto& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::string>) return "string";
        else if constexpr (std::is_integral_v<T>)     return "int";
        else                                          return "double";
    }, var);
    assert(desc == "double");

    // tuple + apply + structured bindings
    const std::tuple t{1, 2.5, 'c'};
    static_assert(std::tuple_size_v<decltype(t)> == 3);
    const auto& [i, dd, c] = t;
    assert(i == 1 && dd == 2.5 && c == 'c');
    assert(std::apply([](int a, double b, char) { return a + b; }, t) == 3.5);

    // <bit>: the portable versions of what used to be compiler intrinsics.
    static_assert(std::popcount(0b1011u) == 3);
    static_assert(std::countl_zero(std::uint8_t{0b0001'0000}) == 3);
    static_assert(std::has_single_bit(64u) && !std::has_single_bit(63u));
    static_assert(std::bit_ceil(100u) == 128u);
    static_assert(std::bit_width(255u) == 8);

    // random, done correctly: never rand() % n (biased, and poor quality).
    std::mt19937 gen{12345};                            // seeded for reproducibility
    std::uniform_int_distribution<int> dist{1, 6};
    int rolls[6]{};
    for (int i = 0; i < 6000; ++i) ++rolls[dist(gen) - 1];
    for (int n : rolls) assert(n > 800 && n < 1200);     // roughly uniform

    // chrono: steady_clock for DURATIONS, system_clock only for wall time.
    using namespace std::chrono_literals;
    constexpr auto period = 10ms;
    static_assert(std::chrono::duration_cast<std::chrono::microseconds>(period).count()
                  == 10'000);
}

int main() {
    views();
    containers();
    vector_beats_list();
    custom_iterator();
    algorithms();
    ranges_demo();
    utilities();
    std::cout << "section 12: all checks passed\n";
}
