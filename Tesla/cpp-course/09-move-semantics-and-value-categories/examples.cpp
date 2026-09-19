// 09 — Move semantics, value categories, forwarding, copy elision.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex
// Experiment: rebuild with -fno-elide-constructors and run it again. The
// copy_elision() asserts will FAIL -- that is the point. Without elision the
// returns become real move constructions, which is exactly what C++17 guaranteed
// away for prvalues and what NRVO removes for named locals.
//   g++ -std=c++20 -fno-elide-constructors -w examples.cpp -o ex_noelide && ./ex_noelide

#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// A type that reports exactly which special member ran.
struct Tracer {
    static int ctor, copy_ctor, move_ctor, copy_assign, move_assign, dtor;
    std::string tag;

    Tracer(std::string t = "") : tag{std::move(t)} { ++ctor; }
    Tracer(const Tracer& o) : tag{o.tag}            { ++copy_ctor; }
    Tracer(Tracer&& o) noexcept : tag{std::move(o.tag)} { ++move_ctor; }
    Tracer& operator=(const Tracer& o) { tag = o.tag;            ++copy_assign; return *this; }
    Tracer& operator=(Tracer&& o) noexcept { tag = std::move(o.tag); ++move_assign; return *this; }
    ~Tracer() { ++dtor; }

    static void reset() { ctor = copy_ctor = move_ctor = copy_assign = move_assign = dtor = 0; }
};
int Tracer::ctor = 0, Tracer::copy_ctor = 0, Tracer::move_ctor = 0,
    Tracer::copy_assign = 0, Tracer::move_assign = 0, Tracer::dtor = 0;

// ---------------------------------------------------------- value categories
static int  by_value(Tracer)        { return 0; }
static void lvalue_only(Tracer&)    {}
static int  const_ref(const Tracer&) { return 1; }
static int  rvalue_only(Tracer&&)    { return 2; }

static void value_categories() {
    Tracer t;
    int x = 0;
    int& lr = x;
    int&& rr = 5;                          // 5 is a prvalue; rr is a NAME

    // decltype tells you the category: T& for lvalue, T&& for xvalue, T for prvalue.
    static_assert(std::is_same_v<decltype((x)),  int&>);       // lvalue
    static_assert(std::is_same_v<decltype((lr)), int&>);       // lvalue
    static_assert(std::is_same_v<decltype((rr)), int&>);       // LVALUE! it has a name
    static_assert(std::is_same_v<decltype(std::move(x)), int&&>);  // xvalue
    static_assert(std::is_same_v<decltype(x + 1), int>);       // prvalue

    lvalue_only(t);                        // t is an lvalue
    assert(const_ref(t) == 1);             // const& takes anything
    assert(const_ref(Tracer{}) == 1);
    assert(rvalue_only(Tracer{}) == 2);    // prvalue
    assert(rvalue_only(std::move(t)) == 2);// xvalue
    // rvalue_only(t);                     // ERROR: t is an lvalue
    (void)by_value;
}

// THE trap: inside this function `s` is a NAMED rvalue reference, i.e. an LVALUE.
static int forwards_wrong(Tracer&& s) { return const_ref(s); }          // copies later
static int forwards_right(Tracer&& s) { return rvalue_only(std::move(s)); }

static void named_rvalue_is_lvalue() {
    Tracer::reset();
    Tracer a{"a"};
    assert(forwards_wrong(std::move(a)) == 1);    // picked the const& overload
    Tracer b{"b"};
    assert(forwards_right(std::move(b)) == 2);    // picked the && overload
}

// -------------------------------------------------------- what a move costs
static void move_vs_copy() {
    Tracer::reset();
    {
        Tracer src{"payload"};
        Tracer copied = src;                  // copy ctor
        Tracer moved  = std::move(src);       // move ctor
        assert(Tracer::copy_ctor == 1 && Tracer::move_ctor == 1);
        assert(copied.tag == "payload" && moved.tag == "payload");
        // A moved-from object is VALID but UNSPECIFIED. In every real
        // implementation std::string leaves it empty; do not RELY on the value,
        // only on being able to reassign or destroy it.
        src = Tracer{"reused"};               // move-assign: perfectly legal
        assert(src.tag == "reused");
    }

    // const kills moves: const T&& binds to const T&, so this COPIES, silently.
    Tracer::reset();
    const Tracer immutable{"c"};
    Tracer from_const = std::move(immutable);        // <- looks like a move
    assert(Tracer::copy_ctor == 1 && Tracer::move_ctor == 0);   // it was a COPY
    assert(from_const.tag == "c");

    // std::move generates NO code -- it is a static_cast. Proof: it is constexpr
    // and usable in a constant expression.
    static_assert(std::move(5) == 5);

    // std::exchange is the idiomatic "steal and reset" for a move ctor.
    int a = 1;
    assert(std::exchange(a, 2) == 1 && a == 2);
}

// noexcept moves matter: vector uses move_if_noexcept when it reallocates.
struct ThrowingMove {
    std::vector<int> d_{1, 2, 3};
    ThrowingMove() = default;
    ThrowingMove(const ThrowingMove&) = default;
    ThrowingMove(ThrowingMove&& o) : d_{std::move(o.d_)} {}   // NOT noexcept
};
struct NoexceptMove {
    std::vector<int> d_{1, 2, 3};
};

static void noexcept_moves() {
    static_assert(!std::is_nothrow_move_constructible_v<ThrowingMove>);
    static_assert(std::is_nothrow_move_constructible_v<NoexceptMove>);
    // The consequence: growing a vector<ThrowingMove> COPIES every element to keep
    // the strong exception guarantee; a vector<NoexceptMove> moves them. Same code,
    // different cost, decided entirely by that noexcept.
    std::vector<ThrowingMove> slow;
    std::vector<NoexceptMove> fast;
    for (int i = 0; i < 100; ++i) { slow.emplace_back(); fast.emplace_back(); }
    assert(slow.size() == 100 && fast.size() == 100);
}

// ------------------------------------------------- forwarding references
template <typename T> constexpr int probe(T&&) {          // FORWARDING reference
    if constexpr (std::is_lvalue_reference_v<T>) return 1; // T deduced as U&
    else                                         return 2; // T deduced as U
}
template <typename T> constexpr int not_forwarding(std::vector<T>&&) { return 3; }

// Perfect forwarding: preserve type AND value category through a wrapper.
static int overload_target(const Tracer&) { return 1; }
static int overload_target(Tracer&&)      { return 2; }

template <typename T> int relay_badly(T&& x)  { return overload_target(x); }
template <typename T> int relay_well(T&& x)   { return overload_target(std::forward<T>(x)); }

template <typename T, typename... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

static void forwarding() {
    Tracer t;
    static_assert(probe(1) == 2);                // prvalue  -> T = int
    assert(probe(t) == 1);                        // lvalue   -> T = Tracer&
    assert(probe(std::move(t)) == 2);             // xvalue   -> T = Tracer
    assert(not_forwarding(std::vector<int>{}) == 3);
    // not_forwarding(lvalue_vector);             // ERROR: vector<T>&& is a plain
    //                                            // rvalue reference, not forwarding

    Tracer a;
    assert(relay_badly(std::move(a)) == 1);       // category LOST -> const& overload
    Tracer b;
    assert(relay_well(std::move(b)) == 2);        // category preserved -> && overload
    assert(relay_well(b) == 1);                   // lvalue stays an lvalue

    // Reference collapsing, spelled out.
    using L = Tracer&;
    using R = Tracer&&;
    static_assert(std::is_same_v<L&,  Tracer&>);   // T&  &  -> T&
    static_assert(std::is_same_v<L&&, Tracer&>);   // T&  && -> T&
    static_assert(std::is_same_v<R&,  Tracer&>);   // T&& &  -> T&
    static_assert(std::is_same_v<R&&, Tracer&&>);  // T&& && -> T&&

    auto p = my_make_unique<Tracer>("forwarded");
    assert(p->tag == "forwarded");
}

// A forwarding-reference constructor is GREEDY: it beats the copy constructor.
class Greedy {
    std::string name_;
public:
    // Unconstrained, this template would win over the copy ctor for `Greedy g2{g1}`
    // because Greedy& is an exact match while the copy ctor needs const Greedy&.
    template <typename S>
        requires (!std::same_as<std::remove_cvref_t<S>, Greedy>)   // <- the fix
    explicit Greedy(S&& s) : name_{std::forward<S>(s)} {}
    Greedy(const Greedy&) = default;
    Greedy(Greedy&&) noexcept = default;
    const std::string& name() const { return name_; }
};

static void greedy_constructor() {
    Greedy g1{"one"};
    Greedy g2{g1};                    // copy ctor, thanks to the constraint
    assert(g1.name() == "one" && g2.name() == "one");
}

// ------------------------------------------------------ copy elision / RVO
static Tracer make_prvalue()          { return Tracer{"prvalue"}; }       // RVO: mandatory
static Tracer make_named()            { Tracer t{"named"}; return t; }    // NRVO: optional
static Tracer make_pessimized()       { Tracer t{"pess"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
                                        return std::move(t); }            // defeats NRVO
#pragma GCC diagnostic pop
// Two locals, so NRVO cannot apply. The two returns behave DIFFERENTLY:
static Tracer make_ternary(bool flag) {
    Tracer a{"a"}, b{"b"};
    return flag ? a : b;      // the operand is not an id-expression, so the
}                             // implicit-move rule does NOT apply -> COPY
static Tracer make_two_returns(bool flag) {
    Tracer a{"a"}, b{"b"};
    if (flag) return a;       // each operand IS a name of a local -> implicit MOVE
    return b;
}

static void copy_elision() {
    Tracer::reset();
    Tracer r1 = make_prvalue();
    assert(Tracer::ctor == 1 && Tracer::copy_ctor == 0 && Tracer::move_ctor == 0);
    assert(r1.tag == "prvalue");             // guaranteed since C++17: ZERO extra ops

    Tracer::reset();
    Tracer r2 = make_named();
    assert(Tracer::ctor == 1 && Tracer::copy_ctor == 0);
    assert(Tracer::move_ctor == 0);          // NRVO (any real compiler at -O0 too)
    assert(r2.tag == "named");

    Tracer::reset();
    Tracer r3 = make_pessimized();
    assert(Tracer::move_ctor == 1);          // the explicit std::move FORCED a move
    assert(r3.tag == "pess");

    // No NRVO in either case, but the ternary COPIES and the two-return form MOVES:
    // C++20's implicit move on return only applies when the returned expression is
    // an id-expression naming a local. `flag ? a : b` is not one.
    Tracer::reset();
    Tracer r4 = make_ternary(true);
    assert(Tracer::copy_ctor == 1 && Tracer::move_ctor == 0);
    assert(r4.tag == "a");

    Tracer::reset();
    Tracer r5 = make_two_returns(true);
    assert(Tracer::move_ctor == 1 && Tracer::copy_ctor == 0);
    assert(r5.tag == "a");

    // Passing a prvalue straight into a by-value parameter: zero copies, zero moves.
    Tracer::reset();
    (void)by_value(make_prvalue());
    assert(Tracer::copy_ctor == 0 && Tracer::move_ctor == 0);
}

// ------------------------------------------------------------- ref-qualifiers
class Builder {
    std::string s_;
public:
    Builder& add(char c) &  { s_ += c; return *this; }
    Builder  add(char c) && { s_ += c; return std::move(*this); }
    const std::string& str() const& { return s_; }          // lvalue: lend a view
    std::string        str() &&     { return std::move(s_); } // rvalue: steal it
};

static void ref_qualifiers() {
    Builder b;
    b.add('x').add('y');
    assert(b.str() == "xy");                       // const& overload: no copy, a view

    // The whole chain is an rvalue, so the buffer is moved out instead of copied.
    std::string built = Builder{}.add('a').add('b').str();
    assert(built == "ab");
}

// ----------------------------------------------------------- decay-copy (C++23)
static void decay_copy() {
    std::vector<int> v{1, 2, 3, 1};
    // std::erase(v, v.front()) would pass a REFERENCE into the vector being
    // modified -- it dangles mid-algorithm. Copy the value out first.
    const int first = v.front();                   // C++23 spelling: auto(v.front())
    std::erase(v, first);
    assert((v == std::vector<int>{2, 3}));
}

int main() {
    value_categories();
    named_rvalue_is_lvalue();
    move_vs_copy();
    noexcept_moves();
    forwarding();
    greedy_constructor();
    copy_elision();
    ref_qualifiers();
    decay_copy();
    std::cout << "section 09: all checks passed\n";
}
