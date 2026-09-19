// 04 — Functions, overloading, lambdas, the preprocessor.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ------------------------------------------------------- parameter passing
// span replaces (pointer, length) and keeps size() -- no decay, no separate arg.
static int sum(std::span<const int> values) {
    return std::accumulate(values.begin(), values.end(), 0);
}

// string_view avoids constructing a std::string per call for a literal.
static std::size_t count_char(std::string_view text, char c) {
    return static_cast<std::size_t>(std::count(text.begin(), text.end(), c));
}

// A sink parameter: take by value, then move. The caller decides copy vs. move.
class Message {
    std::string payload_;
public:
    explicit Message(std::string payload) : payload_{std::move(payload)} {}
    const std::string& payload() const noexcept { return payload_; }
};

static void parameter_passing() {
    std::vector<int> v{1, 2, 3, 4};
    std::array<int, 3> a{10, 20, 30};
    int raw[]{5, 5};
    assert(sum(v) == 10);              // all three convert to span<const int>
    assert(sum(a) == 60);
    assert(sum(raw) == 10);
    assert(sum({}) == 0);

    assert(count_char("hello world", 'l') == 3);   // no allocation

    std::string owned = "moved";
    Message m{std::move(owned)};                   // one move, zero copies
    assert(m.payload() == "moved" && owned.empty());

    // The view trap: sv points into a temporary that is already gone.
    //   std::string_view dangling = std::string("temp");   // use-after-free
    // Views are parameters and locals, never members that outlive their buffer.
}

// --------------------------------------------------------------- overloading
namespace ovl {
int  which(int)            { return 1; }
int  which(double)         { return 2; }
int  which(const char*)    { return 3; }
int  which(std::string_view) { return 4; }

// Return type is NOT part of the signature, and top-level const on a by-value
// parameter is ignored -- both of these would be redefinitions:
//   double which(int);
//   int which(const int);

// = delete as an overload-resolution tool: ban the lossy call sites.
void set_timeout(std::chrono::milliseconds) {}
void set_timeout(int)    = delete;      // reject set_timeout(500)
void set_timeout(double) = delete;

// Non-template beats an equally good template.
template <typename T> int pick(T)   { return 100; }
                      int pick(int) { return 200; }
}  // namespace ovl

static void overloading() {
    assert(ovl::which(1) == 1);
    assert(ovl::which(1.0) == 2);
    assert(ovl::which('c') == 1);          // char PROMOTES to int, beats double
    assert(ovl::which("lit") == 3);        // const char* is an exact match;
                                           // string_view needs a user conversion
    assert(ovl::which(std::string_view{"x"}) == 4);
    assert(ovl::pick(1) == 200);           // non-template wins
    assert(ovl::pick(1.0) == 100);

    using namespace std::chrono_literals;
    ovl::set_timeout(500ms);               // ovl::set_timeout(500) -> compile error
}

// Default arguments are evaluated AT THE CALL SITE -- which is what makes
// std::source_location defaults work.
static void trace(std::string_view msg,
                  const std::source_location loc = std::source_location::current()) {
    std::printf("  [%s:%u in %s] %.*s\n", loc.file_name(), loc.line(),
                loc.function_name(), static_cast<int>(msg.size()), msg.data());
}

// ------------------------------------------------------------------ lambdas
static std::function<int()> make_dangling_closure() {
    int local = 42;
    return [&local] { return local; };   // DANGLES. Shown to be recognised, not called.
}

static void lambdas() {
    // Closure class: unique unnamed type, operator() const by default.
    auto plus = [](int a, int b) { return a + b; };
    static_assert(!std::is_same_v<decltype(plus), int (*)(int, int)>);
    assert(plus(2, 3) == 5);

    int x = 1;
    auto by_copy = [x] { return x; };          // snapshot at creation
    auto by_ref  = [&x] { return x; };         // reads the live variable
    x = 99;
    assert(by_copy() == 1 && by_ref() == 99);

    // mutable is required to modify a by-copy capture, because operator() is const.
    auto counter = [n = 0]() mutable { return ++n; };
    assert(counter() == 1 && counter() == 2 && counter() == 3);

    // init-capture moves a unique_ptr into the closure.
    auto owner = [p = std::make_unique<int>(7)] { return *p; };
    assert(owner() == 7);

    // A captureless lambda converts to a plain function pointer (C API bridge).
    int (*fp)(int, int) = [](int a, int b) { return a * b; };
    assert(fp(3, 4) == 12);
    // A CAPTURING lambda does not:
    //   int (*bad)(int) = [x](int a) { return a + x; };   // error

    // Generic lambda (C++14) and explicit template parameter (C++20).
    auto twice     = [](auto v) { return v + v; };
    auto first_of  = []<typename T>(const std::vector<T>& v) { return v.front(); };
    assert(twice(2) == 4);
    assert(twice(std::string{"ab"}) == "abab");
    assert(first_of(std::vector{9, 8}) == 9);

    // constexpr lambda used in a constant expression.
    constexpr auto cube = [](int v) constexpr { return v * v * v; };
    static_assert(cube(3) == 27);

    // Recursion pre-C++23: pass yourself in.
    auto fact = [](auto&& self, int n) -> long { return n <= 1 ? 1 : n * self(self, n - 1); };
    assert(fact(fact, 5) == 120);

    // The idiom for a complex const initialization: an immediately-invoked lambda.
    const auto squares = [] {
        std::array<int, 8> t{};
        for (std::size_t i = 0; i < t.size(); ++i) t[i] = static_cast<int>(i * i);
        return t;
    }();
    assert(squares[3] == 9);

    // Why a template parameter beats std::function in a hot path: the callable's
    // TYPE is known, so the call inlines. std::function type-erases it.
    auto apply_template = []<typename F>(F&& f, int v) { return f(v); };
    std::function<int(int)> erased = [](int v) { return v * 2; };
    assert(apply_template([](int v) { return v * 2; }, 21) == 42);   // inlined
    assert(erased(21) == 42);                                        // indirect call
    static_assert(sizeof(erased) >= sizeof(void*));                  // it has state

    (void)make_dangling_closure;    // never call it
}

// --------------------------------------------------------------- lambdas: this
class Sensor : public std::enable_shared_from_this<Sensor> {
    std::vector<int> buffer_{1, 2, 3};
public:
    // BAD: [this] captures a raw pointer; if *this dies before the callback runs,
    // the callback is a use-after-free.
    std::function<int()> unsafe_callback() {
        return [this] { return static_cast<int>(buffer_.size()); };
    }
    // GOOD: keep the object alive for exactly as long as the callback exists.
    std::function<int()> safe_callback() {
        return [self = shared_from_this()] { return static_cast<int>(self->buffer_.size()); };
    }
};

static void lambda_this() {
    std::function<int()> cb;
    {
        auto s = std::make_shared<Sensor>();
        cb = s->safe_callback();       // the closure owns a share of *s
    }                                  // s goes out of scope here...
    assert(cb() == 3);                 // ...but the object is still alive
}

// ------------------------------------------------------------- preprocessor
#define BAD_SQUARE(x) x* x                 // no parens: wrong for expressions
#define SQUARE(x) ((x) * (x))              // parenthesized, but STILL double-evaluates
constexpr int square(int v) { return v * v; }   // what you should actually write

#define STRINGIZE_(x) #x
#define STRINGIZE(x) STRINGIZE_(x)         // expand first, then stringize
#define CONCAT(a, b) a##b
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s at %s:%d\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            std::abort();                                                      \
        }                                                                      \
    } while (0)                            /* single statement, needs a ; */

// Double evaluation, demonstrated WITHOUT relying on UB: ?: sequences its
// operands, so this is well defined -- and `a` is still evaluated twice.
#define MAX2(a, b) ((a) < (b) ? (b) : (a))
static int side_effect_calls = 0;
static int next_value() { return ++side_effect_calls; }

#define VERSION 3

static void preprocessor() {
    assert(BAD_SQUARE(1 + 2) == 1 + 2 * 1 + 2);   // == 5, not 9
    assert(SQUARE(1 + 2) == 9);
    static_assert(square(1 + 2) == 9);            // and it cannot double-evaluate

    // A macro evaluates its argument once per textual occurrence. Here next_value()
    // runs twice for one MAX2 call; std::max would call it once.
    side_effect_calls = 0;
    const int m = MAX2(next_value(), 0);
    assert(side_effect_calls == 2 && m == 2);     // "max(1, 0)" returned 2 (!)
    side_effect_calls = 0;
    const int good = std::max(next_value(), 0);
    assert(side_effect_calls == 1 && good == 1);
    // And `SQUARE(i++)` would be outright UB: two unsequenced modifications of i
    // inside one expression. -Wsequence-point catches it.

    int CONCAT(my_, var) = 5;
    assert(my_var == 5);
    assert(std::string_view{STRINGIZE(VERSION)} == "3");    // expanded: "3"
    assert(std::string_view{STRINGIZE_(VERSION)} == "VERSION");  // not expanded

    CHECK(1 + 1 == 2);

#if __has_include(<span>)
    static_assert(true, "feature detection via __has_include");
#endif
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
    static_assert(true, "std::span is available");
#endif
}

int main() {
    parameter_passing();
    overloading();
    lambdas();
    lambda_this();
    preprocessor();
    trace("default arguments are evaluated at the call site");
    std::cout << "section 04: all checks passed\n";
}
