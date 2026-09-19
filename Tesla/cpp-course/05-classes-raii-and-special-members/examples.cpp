// 05 — Classes, RAII, constructors/destructors, the special member functions.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wreorder -g examples.cpp -o ex && ./ex

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------- RAII
// A ring-buffer slot: acquiring it reserves capacity, destroying it releases the
// slot on EVERY exit path -- early return, exception, break.
class SlotPool {
    int free_{4};
public:
    bool try_acquire() noexcept { if (free_ == 0) return false; --free_; return true; }
    void release() noexcept { ++free_; }
    int  free() const noexcept { return free_; }
};

class Slot {
    SlotPool* pool_;                       // non-owning back-pointer
public:
    explicit Slot(SlotPool& pool) : pool_{&pool} {
        if (!pool_->try_acquire()) throw std::runtime_error("pool exhausted");
    }                                      // no half-constructed object exists
    ~Slot() { if (pool_) pool_->release(); }

    Slot(const Slot&)            = delete; // a slot is unique: move-only
    Slot& operator=(const Slot&) = delete;
    Slot(Slot&& o) noexcept : pool_{std::exchange(o.pool_, nullptr)} {}
    Slot& operator=(Slot&& o) noexcept {
        if (this != &o) {
            if (pool_) pool_->release();
            pool_ = std::exchange(o.pool_, nullptr);
        }
        return *this;
    }
    bool owns() const noexcept { return pool_ != nullptr; }
};

static void raii() {
    SlotPool pool;
    assert(pool.free() == 4);
    {
        Slot a{pool}, b{pool};
        assert(pool.free() == 2);
        Slot moved = std::move(a);          // ownership transferred, not duplicated
        assert(!a.owns() && moved.owns());
        assert(pool.free() == 2);           // still 2: no double acquire
    }
    assert(pool.free() == 4);               // every slot released at scope exit

    // Exception safety comes for free: the throw unwinds through ~Slot.
    try {
        Slot s1{pool}, s2{pool}, s3{pool}, s4{pool};
        assert(pool.free() == 0);
        Slot s5{pool};                      // throws
        assert(false);
    } catch (const std::runtime_error&) {
        assert(pool.free() == 4);           // all four released during unwinding
    }
}

// ----------------------------------------------------- invariants & explicit
class Temperature {
    double kelvin_;                         // invariant: kelvin_ >= 0
public:
    explicit Temperature(double k) : kelvin_{k} {
        if (k < 0.0) throw std::invalid_argument("negative absolute temperature");
    }
    double kelvin()  const noexcept { return kelvin_; }
    double celsius() const noexcept { return kelvin_ - 273.15; }
    explicit operator bool() const noexcept { return kelvin_ > 0.0; }
};

static void invariants() {
    const Temperature t{300.0};
    assert(t.kelvin() == 300.0);
    if (t) { /* explicit operator bool works in a boolean context */ }
    // int x = t;   // error: the conversion is explicit
    try { Temperature bad{-1.0}; (void)bad; assert(false); }
    catch (const std::invalid_argument&) { /* invariant enforced at construction */ }
}

// --------------------------------------------------- construction order
static std::string g_log;

struct Base   { Base()   { g_log += "Base ";   } ~Base()   { g_log += "~Base ";   } };
struct MemA   { MemA()   { g_log += "MemA ";   } ~MemA()   { g_log += "~MemA ";   } };
struct MemB   { MemB()   { g_log += "MemB ";   } ~MemB()   { g_log += "~MemB ";   } };

// -Wreorder fires on purpose below: the init-list order is a lie, the
// declaration order is what happens.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
struct Derived : Base {
    MemA a_;                                // declaration order decides
    MemB b_;
    Derived() : b_{}, a_{} { g_log += "body "; }   // written b_ first on purpose
};
#pragma GCC diagnostic pop

static void construction_order() {
    g_log.clear();
    { Derived d; (void)d; }
    // bases -> members IN DECLARATION ORDER -> body; destruction exactly reversed
    assert(g_log == "Base MemA MemB body ~MemB ~MemA ~Base ");
}

// Virtual dispatch does NOT work from a constructor: the derived part does not
// exist yet, so the base's own override is called.
struct VBase {
    VBase() { tag_ = describe(); }                 // calls VBase::describe!
    virtual ~VBase() = default;
    virtual const char* describe() const { return "base"; }
    const char* tag_{};
};
struct VDerived : VBase {
    const char* describe() const override { return "derived"; }
};

static void virtual_in_ctor() {
    VDerived d;
    assert(std::string_view{d.tag_} == "base");        // NOT "derived"
    assert(std::string_view{d.describe()} == "derived"); // normal dispatch, later
}

// ------------------------------------------ the special-member generation rules
struct RuleOfZero {                         // owns via RAII members: declare nothing
    std::vector<int>          data_;
    std::unique_ptr<int>      owned_;       // makes the whole type move-only
};
static_assert(std::is_move_constructible_v<RuleOfZero>);
static_assert(!std::is_copy_constructible_v<RuleOfZero>);   // unique_ptr is not copyable

// THE TRAP: declaring a destructor suppresses the implicit MOVE operations. The
// type still looks movable to std::is_move_constructible, because a copy
// constructor happily binds an rvalue -- so "moving" it silently COPIES.
// Counter lets us observe which one actually ran.
struct Counter {
    static int copies, moves;
    Counter() = default;
    Counter(const Counter&)            { ++copies; }
    Counter(Counter&&) noexcept        { ++moves;  }
    Counter& operator=(const Counter&) { ++copies; return *this; }
    Counter& operator=(Counter&&) noexcept { ++moves; return *this; }
    static void reset() { copies = moves = 0; }
};
int Counter::copies = 0;
int Counter::moves  = 0;

struct NoDtor      { Counter c; };                       // rule of zero
struct DeclaresDtor{ Counter c; ~DeclaresDtor() {} };     // move ops suppressed

static void special_member_generation() {
    Counter::reset();
    { NoDtor a; NoDtor b = std::move(a); (void)b; }
    assert(Counter::moves == 1 && Counter::copies == 0);   // really moved

    Counter::reset();
    { DeclaresDtor a; DeclaresDtor b = std::move(a); (void)b; }
    assert(Counter::copies == 1 && Counter::moves == 0);   // silently COPIED
    // Both traits report true, which is exactly why this bug is invisible:
    static_assert(std::is_move_constructible_v<DeclaresDtor>);
    static_assert(std::is_copy_constructible_v<DeclaresDtor>);
    // The fix: declare the moves explicitly (rule of five) or delete the dtor
    // (rule of zero).
}

struct DeclaresMove {                       // declaring a move DELETES the copies
    std::vector<int> data_;
    DeclaresMove() = default;
    DeclaresMove(DeclaresMove&&) noexcept = default;
    DeclaresMove& operator=(DeclaresMove&&) noexcept = default;
};
static_assert(!std::is_copy_constructible_v<DeclaresMove>);

// = default IN the class keeps the type trivial; that is what lets you memcpy it.
struct TrivialInClass { int a; TrivialInClass() = default; };
struct NotTrivial     { int a; NotTrivial(); };
NotTrivial::NotTrivial() = default;                    // out of line: not trivial
static_assert(std::is_trivially_default_constructible_v<TrivialInClass>);
static_assert(!std::is_trivially_default_constructible_v<NotTrivial>);

// ------------------------------------------------ rule of five, written out
class Buffer {
    std::size_t n_{0};
    int*        p_{nullptr};
public:
    Buffer() noexcept = default;
    explicit Buffer(std::size_t n) : n_{n}, p_{new int[n]{}} {}
    ~Buffer() { delete[] p_; }                              // delete[] matches new[]

    Buffer(const Buffer& o) : n_{o.n_}, p_{new int[o.n_]} {  // deep copy
        std::copy(o.p_, o.p_ + o.n_, p_);
    }
    Buffer(Buffer&& o) noexcept
        : n_{std::exchange(o.n_, 0)}, p_{std::exchange(o.p_, nullptr)} {}

    // copy-and-swap: one implementation handles copy AND move assignment, and is
    // self-assignment safe and exception safe for free.
    Buffer& operator=(Buffer o) noexcept { swap(*this, o); return *this; }

    friend void swap(Buffer& a, Buffer& b) noexcept {        // hidden friend: ADL-only
        std::swap(a.n_, b.n_);
        std::swap(a.p_, b.p_);
    }
    std::size_t size() const noexcept { return n_; }
    int& operator[](std::size_t i) noexcept { return p_[i]; }
    const int& operator[](std::size_t i) const noexcept { return p_[i]; }
};

static void rule_of_five() {
    Buffer a{4};
    a[0] = 7;
    Buffer copy = a;                 // deep copy
    copy[0] = 9;
    assert(a[0] == 7 && copy[0] == 9);

    Buffer moved = std::move(a);      // steals the pointer
    assert(moved.size() == 4 && moved[0] == 7 && a.size() == 0);

    Buffer assigned;
    assigned = moved;                 // copy-assign via the by-value parameter
    assert(assigned.size() == 4);
    assigned = std::move(copy);       // move-assign, same operator
    assert(assigned[0] == 9);

    using std::swap;
    swap(assigned, moved);            // found by ADL
    assert(assigned[0] == 7);
}

// -------------------------------------------------------------- class keywords
class Widget {
    static inline int count_ = 0;              // C++17: no out-of-line definition
    mutable std::optional<int> cached_;        // the legitimate use of mutable
    int* owned_;                               // demonstrates SHALLOW const
public:
    static constexpr int kMaxId = 1024;

    Widget() : owned_{new int{1}} { ++count_; }
    ~Widget() { delete owned_; --count_; }
    Widget(const Widget&)            = delete;
    Widget& operator=(const Widget&) = delete;

    static int live() noexcept { return count_; }

    int expensive() const {                    // const method that memoizes
        if (!cached_) cached_ = 42;             // legal only because cached_ is mutable
        return *cached_;
    }
    void shallow_const_demo() const {
        *owned_ = 99;        // LEGAL in a const method: the POINTER is const,
    }                        // the pointee is not. "const is shallow."
    int owned_value() const noexcept { return *owned_; }
};

static void class_keywords() {
    assert(Widget::live() == 0);
    {
        Widget w;
        assert(Widget::live() == 1);
        assert(w.expensive() == 42 && w.expensive() == 42);
        w.shallow_const_demo();
        assert(w.owned_value() == 99);         // mutated through a const method
        static_assert(Widget::kMaxId == 1024);
    }
    assert(Widget::live() == 0);
}

int main() {
    raii();
    invariants();
    construction_order();
    virtual_in_ctor();
    special_member_generation();
    rule_of_five();
    class_keywords();
    std::cout << "section 05: all checks passed\n";
}
