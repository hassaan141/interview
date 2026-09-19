// 03 — Memory: pointers, references, stack/heap, init, const/constexpr, casts,
//      alignment. The highest-yield file in this repo.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex
//   g++ -std=c++20 -g -fsanitize=address,undefined examples.cpp -o ex && ./ex

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// -------------------------------------------------------- pointers vs. refs
static void pointers_and_references() {
    int x = 42;
    int* p = &x;
    int& r = x;

    *p = 7;
    assert(x == 7 && r == 7);
    r = 9;
    assert(x == 9 && *p == 9);

    // A reference has no identity of its own: sizeof gives the referent's size.
    static_assert(sizeof(p) == sizeof(void*));
    static_assert(sizeof(r) == sizeof(int));

    // Pointer arithmetic is in units of the pointee.
    int a[5]{0, 1, 2, 3, 4};
    assert(*(a + 3) == 3 && a[3] == 3 && 3[a] == 3);   // all identical
    int* end = a + 5;                    // one-past-the-end: legal to form,
    assert(end - a == 5);                // illegal to dereference
    // int* bad = a + 6;                 // UB just to FORM this pointer

    // nullptr has its own type, so it resolves overloads correctly.
    struct Ovl {
        static int f(int)   { return 1; }
        static int f(char*) { return 2; }
    };
    assert(Ovl::f(0) == 1);              // 0 is an int
    assert(Ovl::f(nullptr) == 2);        // nullptr_t -> char*

    // void* round trip needs an explicit cast back.
    void* v = p;
    assert(*static_cast<int*>(v) == 9);
}

// ------------------------------------------------------- reference lifetime
struct Widget {
    std::string name{"widget"};
    const std::string& get_name() const { return name; }    // fine: member outlives
};

static Widget make_widget() { return Widget{}; }

static void reference_lifetime() {
    // Binding a temporary to a const& extends its lifetime to the reference's.
    const std::string& kept = std::string("temporary");
    assert(kept == "temporary");                   // still alive here

    // Lifetime extension applies through the temporary itself, not through a
    // function that RETURNS a reference into it:
    //   const std::string& dead = make_widget().get_name();   // DANGLES
    // The Widget temporary dies at the end of the full-expression; the returned
    // reference points into it. Copy instead:
    const std::string alive = make_widget().get_name();
    assert(alive == "widget");

    // Reallocation invalidates every pointer/reference/iterator into a vector.
    std::vector<int> v{1, 2, 3};
    int* first = &v[0];
    v.reserve(1000);                   // reallocation happens here
    // *first is now dangling -- reading it is use-after-free (ASan catches it)
    (void)first;
    first = &v[0];                     // re-acquire after any reallocation
    assert(*first == 1);
}

// -------------------------------------------------------------- stack / heap
struct Tracked {
    static int live;
    int id;
    explicit Tracked(int i) : id{i} { ++live; }
    ~Tracked() { --live; }
};
int Tracked::live = 0;

static void stack_and_heap() {
    assert(Tracked::live == 0);
    {
        Tracked automatic{1};                     // stack: destroyed at scope exit
        assert(Tracked::live == 1);
    }
    assert(Tracked::live == 0);

    // Raw new/delete, shown so you can recognise it. Do not write this.
    Tracked* raw = new Tracked{2};
    assert(Tracked::live == 1);
    delete raw;
    assert(Tracked::live == 0);
    delete static_cast<Tracked*>(nullptr);        // deleting nullptr is a no-op

    // What you should write instead:
    auto owned = std::make_unique<Tracked>(3);
    assert(Tracked::live == 1);
    owned.reset();
    assert(Tracked::live == 0);

    // Non-throwing allocation returns nullptr instead of throwing bad_alloc.
    int* maybe = new (std::nothrow) int{5};
    assert(maybe != nullptr && *maybe == 5);
    delete maybe;

    // Placement new: construct into storage you already own. This is how a
    // fixed-capacity container / object pool avoids allocating in a hot loop.
    alignas(Tracked) std::byte buffer[sizeof(Tracked)];
    Tracked* in_place = new (buffer) Tracked{4};
    assert(Tracked::live == 1 && in_place->id == 4);
    in_place->~Tracked();                          // manual destruction required
    assert(Tracked::live == 0);
}

// ------------------------------------------------------------ initialization
struct Agg { int x; int y = 2; };

static void initialization() {
    int value_initialized{};                       // 0
    assert(value_initialized == 0);
    // int indeterminate;  <- reading this is UB. -Wuninitialized / MSan catch it.

    Agg a1{};                                      // x == 0, y == 2
    Agg a2{1, 5};
    Agg a3{.x = 7};                                // C++20 designated init
    assert(a1.x == 0 && a1.y == 2);
    assert(a2.x == 1 && a2.y == 5);
    assert(a3.x == 7 && a3.y == 2);

    // THE classic: parentheses vs. braces on a vector.
    std::vector<int> paren(3, 0);                  // 3 elements, all 0
    std::vector<int> brace{3, 0};                  // 2 elements: {3, 0}
    assert(paren.size() == 3 && brace.size() == 2 && brace[0] == 3);

    // Braced init forbids narrowing. These would be compile errors:
    //   int narrow{3.5};   char small{300};
    int truncated = static_cast<int>(3.5);         // explicit = intentional
    assert(truncated == 3);

    // Most vexing parse: `Agg mvp();` declares a FUNCTION. Braces avoid it.
    Agg mvp{};
    assert(mvp.x == 0);

    // Structured bindings.
    auto [x, y] = a2;
    assert(x == 1 && y == 5);
    auto& [rx, ry] = a2;
    rx = 100;
    assert(a2.x == 100);                           // binds by reference
}

// ------------------------------------------------- const / constexpr family
constexpr int square(int v) { return v * v; }      // may run at either time
consteval int must_be_compile_time(int v) { return v + 1; }

constinit int startup_counter = square(3);         // constant-initialized, mutable

static void const_and_constexpr() {
    int x = 1, y = 2;
    const int*       ptr_to_const  = &x;           // can't write *ptr_to_const
    int* const       const_ptr     = &x;           // can't reseat const_ptr
    const int* const both          = &x;
    ptr_to_const = &y;                             // reseating IS allowed
    *const_ptr   = 5;                              // writing IS allowed
    assert(x == 5 && *both == 5 && *ptr_to_const == 2);

    static_assert(square(4) == 16);                // compile time
    const int runtime = square(startup_counter);   // runtime, same function
    assert(runtime == 81);
    static_assert(must_be_compile_time(1) == 2);

    // if constexpr: the discarded branch is not instantiated at all.
    auto describe = []<typename T>(T) {
        if constexpr (std::is_integral_v<T>)       return "integral";
        else if constexpr (std::is_floating_point_v<T>) return "floating";
        else                                        return "other";
    };
    assert(std::string_view{describe(1)} == "integral");
    assert(std::string_view{describe(1.0)} == "floating");

    // constexpr on a variable implies const; on a member function it does not.
    constexpr int k = 10;
    static_assert(std::is_const_v<decltype(k)>);
}

// --------------------------------------------------------------------- casts
struct Base    { virtual ~Base() = default; };
struct Derived : Base { int tag = 7; };
struct Other   : Base {};

static void casts() {
    // static_cast: related types, unchecked downcast.
    Derived d;
    Base*   b  = &d;                                     // implicit upcast
    auto*   d2 = static_cast<Derived*>(b);               // unchecked: I promise
    assert(d2->tag == 7);

    // dynamic_cast: runtime-checked, needs a polymorphic type, costs a lookup.
    Other o;
    Base* wrong = &o;
    assert(dynamic_cast<Derived*>(wrong) == nullptr);    // safely reports failure
    assert(dynamic_cast<Derived*>(b) != nullptr);
    try {
        (void)dynamic_cast<Derived&>(*wrong);            // reference form throws
        assert(false);
    } catch (const std::bad_cast&) { /* expected */ }

    // const_cast: only adds/removes cv. Writing through it to an object that was
    // ORIGINALLY const is UB.
    int mutable_obj = 1;
    const int& cref = mutable_obj;
    const_cast<int&>(cref) = 2;                          // legal: object is not const
    assert(mutable_obj == 2);

    // bit_cast: the correct way to reinterpret an object representation.
    constexpr auto bits = std::bit_cast<std::uint32_t>(1.0f);
    static_assert(bits == 0x3F800000u);
    // reinterpret_cast<uint32_t&>(f) would be a strict-aliasing violation (UB).

    float f = -2.5f;
    std::uint32_t raw{};
    std::memcpy(&raw, &f, sizeof raw);                   // also correct
    assert((raw >> 31) == 1u);                           // sign bit set
}

// A cast worth writing yourself: loud on information loss.
template <typename To, typename From>
constexpr To narrow_cast(From v) {
    const auto out = static_cast<To>(v);
    assert(static_cast<From>(out) == v && "narrow_cast lost information");
    return out;
}

// ------------------------------------------------------- sizeof / alignment
struct BadLayout  { char a; double b; char c; };     // 1+7pad, 8, 1+7pad = 24
struct GoodLayout { double b; char a; char c; };     // 8, 1, 1, 6pad     = 16
struct Empty {};
struct WithEmptyBase : Empty { int x; };

static void layout() {
    static_assert(sizeof(BadLayout)  == 24);
    static_assert(sizeof(GoodLayout) == 16);
    static_assert(alignof(GoodLayout) == 8);
    static_assert(sizeof(Empty) == 1);                  // distinct addresses
    static_assert(sizeof(WithEmptyBase) == 4);          // empty base optimization

    // sizeof(struct) is always a multiple of alignof(struct) so arrays stay aligned.
    static_assert(sizeof(GoodLayout) % alignof(GoodLayout) == 0);

    // An array parameter decays: sizeof inside the callee gives the POINTER size.
    // -Wsizeof-array-argument fires here on purpose: that warning IS the lesson.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-array-argument"
    auto wrong_size = [](int arr[10]) { return sizeof(arr); };
#pragma GCC diagnostic pop
    int arr[10]{};
    assert(wrong_size(arr) == sizeof(int*));
    assert(sizeof(arr) == 10 * sizeof(int));            // only correct at the definition

    // sizeof is unevaluated: this never calls the function.
    auto never_called = []() -> int { std::abort(); };
    static_assert(sizeof(never_called()) == sizeof(int));

    // Padding to a cache line to avoid false sharing between two hot counters.
    struct alignas(64) Counter { std::int64_t n; };
    static_assert(sizeof(Counter) == 64);
    static_assert(alignof(Counter) == 64);
}

int main() {
    pointers_and_references();
    reference_lifetime();
    stack_and_heap();
    initialization();
    const_and_constexpr();
    casts();
    layout();
    assert(narrow_cast<int>(42L) == 42);
    std::cout << "section 03: all checks passed\n";
}
