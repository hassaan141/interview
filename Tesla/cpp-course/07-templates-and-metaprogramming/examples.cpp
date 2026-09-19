// 07 — Templates, traits, SFINAE, concepts, variadics.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ------------------------------------------------------- function templates
template <typename T>
constexpr const T& max_of(const T& a, const T& b) { return a < b ? b : a; }

// Function templates cannot be PARTIALLY specialized. Overloads can be more
// specialized, and take part in partial ordering:
template <typename T> constexpr int rank(T)   { return 0; }   // primary
template <typename T> constexpr int rank(T*)  { return 1; }   // more specialized
template <typename T> constexpr int rank(T**) { return 2; }   // most specialized
template <>           constexpr int rank(char) { return 10; } // full specialization of #1
constexpr int rank(double) { return 20; }                      // plain fn: wins ties

static void function_templates() {
    static_assert(max_of(1, 2) == 2);
    static_assert(max_of<double>(1, 2.5) == 2.5);   // explicit: both convert
    // max_of(1, 2.5);   // ERROR: T deduced int from #1 and double from #2 --
    //                   // deduction never applies conversions.

    int  i{};
    int* p{&i};
    int** pp{&p};
    static_assert(rank(1) == 0);
    assert(rank(p) == 1);
    assert(rank(pp) == 2);
    static_assert(rank('c') == 10);
    static_assert(rank(1.0) == 20);
}

// ---------------------------------------------------------- class templates
template <typename T, std::size_t N>
class FixedArray {
    T data_[N]{};
public:
    static constexpr std::size_t size() noexcept { return N; }
    constexpr T&       operator[](std::size_t i)       { return data_[i]; }
    constexpr const T& operator[](std::size_t i) const { return data_[i]; }
    constexpr T* begin() { return data_; }
    constexpr T* end()   { return data_ + N; }
};

// Partial specialization: a zero-length array holds nothing.
template <typename T>
class FixedArray<T, 0> {
public:
    static constexpr std::size_t size() noexcept { return 0; }
};

// Full specialization: pack bools into bits.
template <>
class FixedArray<bool, 8> {
    std::uint8_t bits_{};
public:
    static constexpr std::size_t size() noexcept { return 8; }
    constexpr bool get(std::size_t i) const { return (bits_ >> i) & 1u; }
    constexpr void set(std::size_t i, bool v) {
        bits_ = static_cast<std::uint8_t>(v ? (bits_ | (1u << i)) : (bits_ & ~(1u << i)));
    }
};

// CTAD: an explicit deduction guide.
template <typename T> struct Wrapper { T value; };
template <typename T> Wrapper(T) -> Wrapper<T>;

static void class_templates() {
    FixedArray<int, 3> a;
    a[0] = 7;
    static_assert(a.size() == 3);
    assert(a[0] == 7);
    static_assert(sizeof(FixedArray<int, 3>) == 12);     // no pointer, no padding

    static_assert(FixedArray<int, 0>::size() == 0);      // partial specialization
    FixedArray<bool, 8> bits;                            // full specialization
    bits.set(3, true);
    assert(bits.get(3) && !bits.get(4));
    static_assert(sizeof(FixedArray<bool, 8>) == 1);

    Wrapper w{1.5};                                      // CTAD -> Wrapper<double>
    static_assert(std::is_same_v<decltype(w), Wrapper<double>>);
    std::vector v{1, 2, 3};                              // CTAD -> vector<int>
    static_assert(std::is_same_v<decltype(v), std::vector<int>>);
}

// ------------------------------------------------- dependent names, two phases
template <typename Container>
typename Container::value_type first_element(const Container& c) {
    //  ^ `typename` required: value_type is a DEPENDENT name and the compiler
    //    cannot know it is a type without being told.
    return *c.begin();
}

template <typename T> struct BaseT { void helper() {} int value{5}; };
template <typename T> struct DerivedT : BaseT<T> {
    int f() {
        // helper();          // ERROR: unqualified lookup does not search a
        this->helper();       // dependent base. Use this-> or qualify.
        return BaseT<T>::value;
    }
};

static void dependent_names() {
    assert(first_element(std::vector{4, 5, 6}) == 4);
    assert(first_element(std::list<std::string>{"a"}) == "a");
    DerivedT<int> d;
    assert(d.f() == 5);
}

// -------------------------------------------------------- decltype subtleties
static void decltype_rules() {
    int x = 0;
    int& rx = x;
    static_assert(std::is_same_v<decltype(x), int>);
    static_assert(std::is_same_v<decltype((x)), int&>);   // extra parens -> lvalue!
    static_assert(std::is_same_v<decltype(rx), int&>);
    static_assert(std::is_same_v<decltype(x + 1), int>);  // prvalue -> plain int

    // auto drops references; decltype(auto) keeps them.
    auto           by_value = rx;   static_assert(std::is_same_v<decltype(by_value), int>);
    decltype(auto) by_ref   = rx;   static_assert(std::is_same_v<decltype(by_ref), int&>);
    (void)by_value; (void)by_ref;

    // declval fabricates a T&& for unevaluated contexts (never call it).
    static_assert(std::is_same_v<decltype(std::declval<int>() + std::declval<double>()),
                                 double>);
}

// -------------------------------------------------------- writing your own trait
template <typename T> struct is_ptr            : std::false_type {};
template <typename T> struct is_ptr<T*>        : std::true_type  {};
template <typename T> struct is_ptr<T* const>  : std::true_type  {};
template <typename T>
constexpr bool is_ptr_v = is_ptr<std::remove_volatile_t<T>>::value;

// A detection idiom (the pre-concepts "does T have .size()?").
template <typename T, typename = void> struct has_size : std::false_type {};
template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<const T&>().size())>>
    : std::true_type {};

static void own_traits() {
    static_assert(is_ptr_v<int*>);
    static_assert(is_ptr_v<int* const>);
    static_assert(!is_ptr_v<int>);
    static_assert(has_size<std::vector<int>>::value);
    static_assert(!has_size<int>::value);
}

// ------------------------------------------- TMP vs. constexpr (what to write now)
template <unsigned N> struct FactorialTMP {
    static constexpr unsigned value = N * FactorialTMP<N - 1>::value;
};
template <> struct FactorialTMP<0> { static constexpr unsigned value = 1; };

constexpr unsigned factorial(unsigned n) { return n <= 1 ? 1u : n * factorial(n - 1); }

static void metaprogramming() {
    static_assert(FactorialTMP<5>::value == 120);    // one class per value: slow
    static_assert(factorial(5) == 120);              // one function: fast, readable
    // Same answer; the constexpr function also works at runtime and its error
    // messages are readable.
}

// --------------------------------------------------------------- SFINAE (legacy)
template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr int describe_sfinae(T) { return 1; }
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
constexpr int describe_sfinae(T) { return 2; }
// Expression SFINAE: "has .size()".
template <typename T> constexpr auto size_or_zero(const T& t) -> decltype(t.size()) {
    return t.size();
}
constexpr std::size_t size_or_zero(...) { return 0; }

// --------------------------------------------------------------- C++20 concepts
// NOTE: subsumption works on the NORMALIZED form of a constraint, which is a
// tree of atomic constraints. A concept defined over a type trait
// (`std::is_arithmetic_v<T>`) is ONE opaque atom, unrelated to std::integral's
// atom -- so `template <std::integral T>` would NOT be considered more
// constrained and the overloads below would be AMBIGUOUS. Defining Arithmetic as
// a disjunction of the standard concepts makes the relationship visible to the
// compiler, so `std::integral` subsumes it.
template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

template <typename T>
concept Container = requires(T c) {
    typename T::value_type;                                   // type requirement
    c.begin();                                                // simple requirement
    c.end();
    { c.size() } -> std::convertible_to<std::size_t>;         // compound requirement
    requires std::default_initializable<T>;                   // nested requirement
};

template <typename T>
concept Serializable = requires(const T& t, std::string& out) {
    { t.serialize(out) } -> std::same_as<void>;
};

template <Arithmetic T> constexpr T twice(T v) { return v + v; }

// Constraint subsumption: the MORE constrained overload wins.
template <std::integral T>       constexpr int which(T) { return 1; }
template <std::floating_point T> constexpr int which(T) { return 2; }
template <Arithmetic T>          constexpr int which(T) { return 3; }   // less constrained

template <Container C>
constexpr std::size_t total_size(const C& c) { return c.size(); }

struct Message { void serialize(std::string& out) const { out += "msg"; } };
struct NotAMessage {};

static void concepts_demo() {
    static_assert(twice(2) == 4);
    static_assert(twice(1.5) == 3.0);
    // twice(std::string{});   // one-line error: "constraint not satisfied"

    static_assert(which(1) == 1);        // integral is more constrained than Arithmetic
    static_assert(which(1.0) == 2);

    static_assert(Container<std::vector<int>>);
    static_assert(Container<std::map<int, int>>);
    static_assert(!Container<int>);
    assert(total_size(std::vector{1, 2, 3}) == 3);

    static_assert(Serializable<Message>);
    static_assert(!Serializable<NotAMessage>);

    // The legacy spellings still work and you must recognise them.
    static_assert(describe_sfinae(1) == 1);
    static_assert(describe_sfinae(1.0) == 2);
    assert(size_or_zero(std::vector{1, 2}) == 2);
    assert(size_or_zero(42) == 0);
}

// ------------------------------------------------- variadics & fold expressions
template <typename... Ts> constexpr std::size_t type_count = sizeof...(Ts);

template <typename... Ts> constexpr auto add(Ts... vs) { return (vs + ...); }        // unary right fold
template <typename... Ts> constexpr auto add0(Ts... vs) { return (0 + ... + vs); }   // binary left fold
template <typename... Ts> constexpr bool all_of(Ts... vs) { return (vs && ...); }    // identity: true
template <typename... Ts> constexpr bool any_of(Ts... vs) { return (vs || ...); }    // identity: false

template <typename... Ts> void print_all(const Ts&... vs) {
    ((std::cout << vs << ' '), ...);                     // comma fold
    std::cout << '\n';
}

// Perfect forwarding a pack into a factory.
template <typename T, typename... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// A homogeneous pack, constrained.
template <std::same_as<int>... Ts> constexpr int sum_ints(Ts... vs) { return (vs + ...); }

struct Three { int a, b, c; Three(int x, int y, int z) : a{x}, b{y}, c{z} {} };

static void variadics() {
    static_assert(type_count<int, double, char> == 3);
    static_assert(add(1, 2, 3) == 6);
    static_assert(add0() == 0);                   // binary fold has an init value
    static_assert(all_of() == true);              // && identity on an empty pack
    static_assert(any_of() == false);             // || identity
    static_assert(all_of(true, true, false) == false);
    static_assert(sum_ints(1, 2, 3) == 6);
    // sum_ints(1, 2.0);   // constraint failure: not every element is int

    auto t = my_make_unique<Three>(1, 2, 3);
    assert(t->a == 1 && t->c == 3);
    std::cout << "  fold print: ";
    print_all(1, 2.5, "three");
}

// ----------------------------------------------------------- template debugging
// Uncomment to make the compiler print a deduced type in the error message:
//   template <typename...> struct WhatIs;
//   WhatIs<decltype(some_expression)> probe;

int main() {
    function_templates();
    class_templates();
    dependent_names();
    decltype_rules();
    own_traits();
    metaprogramming();
    concepts_demo();
    variadics();
    std::cout << "section 07: all checks passed\n";
}
