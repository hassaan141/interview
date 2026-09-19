// 10 — Error handling, noexcept, smart pointers.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex
//   Catch real bugs:  g++ -std=c++20 -g -fsanitize=address,undefined ...
//
// std::expected is C++23; a minimal stand-in is provided below so this file
// compiles with -std=c++20 on gcc 13.

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ----------------------------------------------------------- custom exceptions
class SensorError : public std::runtime_error {
    int code_;
public:
    SensorError(int code, const std::string& what)
        : std::runtime_error{what}, code_{code} {}
    int code() const noexcept { return code_; }
};

static void may_throw(int mode) {
    switch (mode) {
        case 0: return;
        case 1: throw SensorError{42, "lidar timeout"};
        case 2: throw std::out_of_range{"index"};
        default: throw 7;                       // throwing a non-exception type
    }
}

static std::string classify(int mode) {
    try {
        may_throw(mode);
        return "ok";
    }
    // Most-derived FIRST: if std::exception came first it would swallow SensorError.
    catch (const SensorError& e) {
        return "sensor:" + std::to_string(e.code()) + ":" + e.what();
    }
    catch (const std::exception& e) {
        return std::string{"std:"} + e.what();
    }
    catch (...) {
        return "unknown";
    }
}

static void exceptions() {
    assert(classify(0) == "ok");
    assert(classify(1) == "sensor:42:lidar timeout");
    assert(classify(2) == "std:index");
    assert(classify(3) == "unknown");

    // RAII means the throw cannot leak: the destructor runs during unwinding.
    struct Guard { bool* released; ~Guard() { *released = true; } };
    bool released = false;
    try {
        Guard g{&released};
        throw std::runtime_error{"boom"};
    } catch (const std::runtime_error&) {}
    assert(released);

    // Adding context without losing the original: nested exceptions.
    try {
        try { throw SensorError{1, "inner"}; }
        catch (...) { std::throw_with_nested(std::runtime_error{"while calibrating"}); }
    } catch (const std::runtime_error& outer) {
        assert(std::string{outer.what()} == "while calibrating");
        try { std::rethrow_if_nested(outer); assert(false); }
        catch (const SensorError& inner) { assert(inner.code() == 1); }
    }
}

// ---------------------------------------------------------------- noexcept
static void never_throws() noexcept {}
static void might_throw() { throw std::runtime_error{"x"}; }

template <typename T>
static void conditional() noexcept(std::is_nothrow_move_constructible_v<T>) {}

static void noexcept_specifier() {
    static_assert(noexcept(never_throws()));      // the noexcept OPERATOR
    static_assert(!noexcept(might_throw()));
    static_assert(noexcept(conditional<int>()));
    static_assert(noexcept(conditional<std::vector<int>>()));   // its move is noexcept

    // Destructors are implicitly noexcept; throwing from one during unwinding
    // calls std::terminate. Never do it -- swallow and log instead.
    struct SafeDtor {
        ~SafeDtor() {
            try { might_throw(); } catch (...) { /* log, never propagate */ }
        }
    };
    { SafeDtor s; (void)s; }

    // std::move_if_noexcept is what vector uses; it is why noexcept matters.
    std::vector<int> v{1, 2, 3};
    auto&& maybe_moved = std::move_if_noexcept(v);
    static_assert(std::is_same_v<decltype(maybe_moved), std::vector<int>&&>);
}

// ------------------------------------------- error returns: optional / expected
// A tiny std::expected stand-in so this file builds with -std=c++20.
template <typename T, typename E>
class Result {
    std::variant<T, E> v_;
public:
    Result(T value) : v_{std::move(value)} {}
    static Result fail(E e) { Result r{}; r.v_ = std::move(e); return r; }

    bool has_value() const noexcept { return v_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }
    const T& value() const { return std::get<0>(v_); }
    const E& error() const { return std::get<1>(v_); }

    template <typename F>
    auto transform(F&& f) const -> Result<decltype(f(value())), E> {
        using U = decltype(f(value()));
        if (has_value()) return Result<U, E>{f(value())};
        return Result<U, E>::fail(error());
    }
private:
    Result() : v_{} {}
};

enum class ParseError : std::uint8_t { TooShort, BadChecksum, UnknownId };

struct Frame { std::uint32_t id{}; std::uint32_t payload{}; };

static Result<Frame, ParseError> parse(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4) return Result<Frame, ParseError>::fail(ParseError::TooShort);
    const std::uint32_t id = bytes[0];
    if (id == 0)          return Result<Frame, ParseError>::fail(ParseError::UnknownId);
    const std::uint8_t sum = static_cast<std::uint8_t>(bytes[0] + bytes[1] + bytes[2]);
    if (sum != bytes[3])  return Result<Frame, ParseError>::fail(ParseError::BadChecksum);
    return Frame{id, bytes[1]};
}

// optional: "absent" needs no explanation.
static std::optional<int> find_index(std::span<const int> haystack, int needle) {
    for (std::size_t i = 0; i < haystack.size(); ++i)
        if (haystack[i] == needle) return static_cast<int>(i);
    return std::nullopt;
}

static void error_returns() {
    const std::uint8_t good[]{1, 2, 3, 6};
    const std::uint8_t bad_crc[]{1, 2, 3, 9};
    const std::uint8_t too_short[]{1};

    auto r = parse(good);
    assert(r && r.value().id == 1 && r.value().payload == 2);
    assert(!parse(bad_crc) && parse(bad_crc).error() == ParseError::BadChecksum);
    assert(!parse(too_short) && parse(too_short).error() == ParseError::TooShort);

    // Composition without a pile of if-statements (std::expected has this built in).
    auto doubled = parse(good).transform([](const Frame& f) { return f.payload * 2u; });
    assert(doubled && doubled.value() == 4u);
    auto failed = parse(bad_crc).transform([](const Frame& f) { return f.payload * 2u; });
    assert(!failed && failed.error() == ParseError::BadChecksum);

    const int data[]{5, 6, 7};
    assert(find_index(data, 6) == 1);
    assert(!find_index(data, 99).has_value());
    assert(find_index(data, 99).value_or(-1) == -1);
    try { (void)find_index(data, 99).value(); assert(false); }
    catch (const std::bad_optional_access&) {}

    // std::error_code: a cheap, non-throwing, extensible error channel (what the
    // filesystem and networking libraries use).
    const std::error_code ec = std::make_error_code(std::errc::no_such_file_or_directory);
    assert(ec.value() == ENOENT && !ec.message().empty());
}

// -------------------------------------------------------------- unique_ptr
struct Counted {
    static int live;
    int id;
    explicit Counted(int i) : id{i} { ++live; }
    ~Counted() { --live; }
};
int Counted::live = 0;

struct LoudDeleter {                        // stateless -> EBO -> no size cost
    void operator()(Counted* p) const noexcept { delete p; }
};

static void unique_ptr_demo() {
    static_assert(sizeof(std::unique_ptr<Counted>) == sizeof(Counted*));
    static_assert(sizeof(std::unique_ptr<Counted, LoudDeleter>) == sizeof(Counted*));
    static_assert(!std::is_copy_constructible_v<std::unique_ptr<Counted>>);
    static_assert(std::is_nothrow_move_constructible_v<std::unique_ptr<Counted>>);

    assert(Counted::live == 0);
    {
        auto a = std::make_unique<Counted>(1);
        assert(Counted::live == 1);
        auto b = std::move(a);                  // ownership transferred
        assert(a == nullptr);                   // GUARANTEED null after a move
        assert(b->id == 1 && Counted::live == 1);

        Counted* raw = b.release();             // we now own it manually
        assert(b == nullptr && Counted::live == 1);
        std::unique_ptr<Counted, LoudDeleter> c{raw};   // adopt it back
        assert(c->id == 1);
    }
    assert(Counted::live == 0);

    // The array form calls delete[] instead of delete. make_unique<T[]>(n)
    // value-initializes, so T needs a default constructor.
    {
        auto arr = std::make_unique<int[]>(3);
        arr[0] = 1;
        assert(arr[0] == 1 && arr[2] == 0);          // value-initialized to 0
    }

    // Wrapping a C resource: the deleter is part of the type.
    auto fp = std::unique_ptr<std::FILE, int (*)(std::FILE*)>{
        std::fopen("/dev/null", "r"), &std::fclose};
    assert(fp != nullptr);
}

// -------------------------------------------------------------- shared_ptr
static void shared_ptr_demo() {
    // Two pointers wide: object + control block.
    static_assert(sizeof(std::shared_ptr<Counted>) == 2 * sizeof(void*));
    static_assert(sizeof(std::weak_ptr<Counted>) == 2 * sizeof(void*));

    assert(Counted::live == 0);
    {
        auto a = std::make_shared<Counted>(7);      // ONE allocation
        assert(a.use_count() == 1);
        {
            auto b = a;                             // atomic increment
            assert(a.use_count() == 2 && b->id == 7);
        }
        assert(a.use_count() == 1);                 // atomic decrement

        std::weak_ptr<Counted> w = a;               // does NOT extend ownership
        assert(w.use_count() == 1 && !w.expired());
        if (auto locked = w.lock()) assert(locked->id == 7);
        a.reset();
        assert(Counted::live == 0 && w.expired());
        assert(w.lock() == nullptr);                // safe: reports the death
    }
}

// A reference cycle leaks; weak_ptr breaks it.
struct Child;
struct Parent {
    std::shared_ptr<Child> child;
    static int live; Parent() { ++live; } ~Parent() { --live; }
};
struct Child {
    std::shared_ptr<Parent> strong_back_ref;   // <- the leak
    std::weak_ptr<Parent>   weak_back_ref;     // <- the fix
    static int live; Child() { ++live; } ~Child() { --live; }
};
int Parent::live = 0;
int Child::live  = 0;

static void reference_cycles() {
    {   // the leak
        auto p = std::make_shared<Parent>();
        auto c = std::make_shared<Child>();
        p->child = c;
        c->strong_back_ref = p;                 // cycle: neither count reaches 0
    }
    assert(Parent::live == 1 && Child::live == 1);   // LEAKED (ASan/LSan reports it)
    // Break it by hand so the rest of the program is clean:
    // (in real code you would never have created the cycle)

    {   // the fix
        auto p = std::make_shared<Parent>();
        auto c = std::make_shared<Child>();
        p->child = c;
        c->weak_back_ref = p;                   // non-owning back-reference
        assert(c->weak_back_ref.use_count() == 1);
    }
    assert(Parent::live == 1 && Child::live == 1);   // only the earlier leak remains
}

// enable_shared_from_this: keep *this alive for the duration of a callback.
class Session : public std::enable_shared_from_this<Session> {
    int value_{99};
public:
    std::function<int()> make_callback() {
        // [this] would dangle if the Session dies before the callback runs.
        return [self = shared_from_this()] { return self->value_; };
    }
};

static void shared_from_this_demo() {
    std::function<int()> cb;
    {
        auto s = std::make_shared<Session>();
        cb = s->make_callback();
    }                                   // s is gone, but the closure holds a share
    assert(cb() == 99);

    // Calling shared_from_this() on a non-shared_ptr-owned object throws.
    try { Session bare; (void)bare.make_callback(); assert(false); }
    catch (const std::bad_weak_ptr&) {}
}

int main() {
    exceptions();
    noexcept_specifier();
    error_returns();
    unique_ptr_demo();
    shared_ptr_demo();
    reference_cycles();
    shared_from_this_demo();
    std::cout << "section 10: all checks passed\n";
    std::cout << "  (reference_cycles() leaks 2 objects ON PURPOSE -- run this file\n"
                 "   under -fsanitize=address and read the LeakSanitizer report.)\n";
}
