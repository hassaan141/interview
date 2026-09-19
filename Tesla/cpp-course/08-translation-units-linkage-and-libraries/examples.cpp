// 08 — Linkage, storage duration, ODR, initialization order (single-TU parts).
// The genuinely multi-TU experiments live in ./linkage_demo -- run ./linkage_demo/build.sh
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex

#include <cassert>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// ------------------------------------------- the four meanings of `static`
static int s_internal_linkage = 1;      // (1) namespace scope: INTERNAL linkage

static int block_scope_static() {
    static int calls = 0;               // (2) block scope: STATIC DURATION,
    return ++calls;                     //     initialized once, thread-safe (C++11)
}

struct Widget {
    static inline int instances = 0;    // (3) one per CLASS, not per object
    static int count() { return instances; }   // (4) member fn with NO `this`
    Widget()  { ++instances; }
    ~Widget() { --instances; }
};

// Anonymous namespace: internal linkage, and unlike `static` it works for TYPES.
namespace {
struct TuLocalTag {};                   // unique to this TU -> no ODR clash
int tu_local = 0;
int bump() { return ++tu_local; }
}  // namespace

// `inline` does NOT mean "inline this call" -- it means "this definition may appear
// in every TU and the linker will merge them". That is what makes it header-safe.
inline int square(int x) { return x * x; }
inline int g_inline_counter = 0;          // C++17 inline variable: header-safe global

constexpr int kInternal = 10;             // namespace-scope const/constexpr has
                                          // INTERNAL linkage in C++ (unlike C)
extern const int kExternal;               // ...unless you say extern
const int kExternal = 20;

static void linkage_and_duration() {
    assert(s_internal_linkage == 1);
    assert(block_scope_static() == 1);
    assert(block_scope_static() == 2);    // the same object across calls
    assert(block_scope_static() == 3);

    assert(Widget::count() == 0);
    { Widget a, b; assert(Widget::count() == 2); }
    assert(Widget::count() == 0);

    assert(bump() == 1 && bump() == 2);
    assert(square(4) == 16);
    ++g_inline_counter;
    assert(g_inline_counter == 1);
    static_assert(kInternal == 10);
    assert(kExternal == 20);
    static_assert(std::is_class_v<TuLocalTag>);
}

// --------------------------------------------- static initialization order fiasco
// Two namespace-scope objects with DYNAMIC initialization. Within one TU the order
// is declaration order; ACROSS TUs it is unspecified -- which is the bug.
struct Config {
    int level;
    Config() : level{3} { std::cout << "  Config constructed\n"; }
};

// The construct-on-first-use idiom: initialized on the first call, in a defined
// order, and thread-safe since C++11 (the compiler emits a guard variable).
static Config& config() {
    static Config c;              // <- the fix for the ordering fiasco
    return c;
}

struct Logger {
    int level;
    Logger() : level{config().level} {}     // safe: config() is constructed on demand
};

static Logger& logger() {
    static Logger l;
    return l;
}

// constinit: constant-initialized (so no ordering problem at all) but still mutable.
constinit int g_startup_flags = 0b1010;

static void initialization_order() {
    assert(logger().level == 3);       // Config was built before Logger, guaranteed
    assert(config().level == 3);
    assert(g_startup_flags == 0b1010);
    g_startup_flags |= 1;             // constinit is mutable, unlike constexpr
    assert(g_startup_flags == 0b1011);
}

// -------------------------------------------- thread_local vs static duration
static int g_shared_counter = 0;              // one for the whole program
thread_local int t_per_thread_counter = 0;    // one per thread

static void thread_storage() {
    std::mutex m;
    auto work = [&] {
        for (int i = 0; i < 1000; ++i) {
            ++t_per_thread_counter;           // no synchronization needed
            std::lock_guard lk{m};
            ++g_shared_counter;               // shared: needs the lock
        }
        // Each thread sees its own copy, so this is always exactly 1000.
        assert(t_per_thread_counter == 1000);
    };
    std::thread a{work}, b{work};
    a.join();
    b.join();
    assert(g_shared_counter == 2000);
    assert(t_per_thread_counter == 0);        // the main thread's own copy
}

// ------------------------------------------- extern "C" and name mangling
// extern "C" suppresses mangling, so the symbol is literally `tesla_tick`. That is
// what makes a stable, cross-language, cross-compiler boundary possible -- and why
// you cannot overload an extern "C" function.
extern "C" int tesla_tick(int n) { return n + 1; }
// int tesla_tick(double);        // ERROR: C linkage cannot be overloaded

static void c_linkage() {
    assert(tesla_tick(1) == 2);
    // Try it yourself:
    //   nm -C ex | grep -E 'tesla_tick|square'
    // tesla_tick appears unmangled; square appears as _Z6squarei (if not inlined).
}

int main() {
    linkage_and_duration();
    initialization_order();
    thread_storage();
    c_linkage();
    std::cout << "section 08: all checks passed\n";
}
