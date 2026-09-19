// 02 — Entities, enums, struct/union/bitfields, control flow, namespaces.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// -------------------------------------------------------------------- enums
enum class State : std::uint8_t { Idle = 0, Driving = 1, Fault = 255 };

// Fixed underlying type => casting any uint8_t in is well defined, but you still
// have to validate, because 42 is not a named enumerator.
constexpr bool is_known(State s) {
    switch (s) {                       // no default: -Wswitch warns if we add an
        case State::Idle:              // enumerator and forget to handle it here
        case State::Driving:
        case State::Fault:  return true;
    }
    return false;
}

static void enums() {
    static_assert(sizeof(State) == 1);
    static_assert(static_cast<std::uint8_t>(State::Fault) == 255);
    assert(is_known(static_cast<State>(1)));
    assert(!is_known(static_cast<State>(42)));      // defined, but not a valid state
    // static_assert(State::Idle == 0);             // error: no implicit conversion
}

// ---------------------------------------------------------------- bitfields
struct CanStatus {
    std::uint32_t id       : 11;
    std::uint32_t rtr      : 1;
    std::uint32_t reserved : 4;
    std::uint32_t dlc      : 4;
};

// The portable equivalent: explicit shifts and masks. Prefer this on the wire.
constexpr std::uint32_t pack_can(std::uint32_t id, bool rtr, std::uint32_t dlc) {
    return (id & 0x7FFu) | (static_cast<std::uint32_t>(rtr) << 11)
           | ((dlc & 0xFu) << 16);
}
constexpr std::uint32_t can_id(std::uint32_t w)  { return w & 0x7FFu; }
constexpr std::uint32_t can_dlc(std::uint32_t w) { return (w >> 16) & 0xFu; }

static void bitfields() {
    CanStatus s{};
    s.id = 0x7FF; s.rtr = 1; s.dlc = 8;
    assert(s.id == 0x7FF && s.dlc == 8);
    static_assert(sizeof(CanStatus) == 4);          // packs into one uint32_t

    // Same logical content, portable across toolchains and endianness choices.
    static_assert(can_id(pack_can(0x123, false, 8)) == 0x123);
    static_assert(can_dlc(pack_can(0x123, false, 8)) == 8);

    // Bitfields you CANNOT do:
    //   &s.id                      // no address of a bitfield
    //   std::uint32_t& r = s.id;   // no non-const reference to a bitfield
    // And: two threads writing s.id and s.dlc is a DATA RACE, because the
    // compiler read-modify-writes the shared 32-bit allocation unit.
}

// --------------------------------------------------------- unions / punning
union Raw { float f; std::uint32_t u; };

static void type_punning() {
    // UB (reading the inactive member) -- shown only so you recognise it:
    //   Raw r; r.f = 1.0f; auto bits = r.u;

    // Correct, constexpr, zero cost:
    constexpr auto bits = std::bit_cast<std::uint32_t>(1.0f);
    static_assert(bits == 0x3F800000u);             // IEEE-754 encoding of 1.0f
    static_assert(std::bit_cast<float>(0x3F800000u) == 1.0f);

    // Correct pre-C++20; compiles to a single move at -O1 and above.
    float f = 2.0f;
    std::uint32_t u{};
    std::memcpy(&u, &f, sizeof u);
    assert(u == 0x40000000u);

    // Tagged alternative to a union: variant knows its active member.
    std::variant<int, float, std::string> v = 3.5f;
    assert(std::holds_alternative<float>(v));
    assert(v.index() == 1);
    try {
        (void)std::get<int>(v);                     // throws, instead of being UB
        assert(false);
    } catch (const std::bad_variant_access&) { /* expected */ }
    const double as_double = std::visit(
        [](auto&& x) -> double {
            if constexpr (std::is_arithmetic_v<std::decay_t<decltype(x)>>)
                return static_cast<double>(x);
            else
                return static_cast<double>(x.size());
        }, v);
    assert(as_double == 3.5);
}

// ------------------------------------------------------------ control flow
static std::vector<int> make_vector() { return {1, 2, 3, 4}; }

struct Holder { std::vector<int> data{10, 20, 30}; };
static Holder make_holder() { return Holder{}; }

static void control_flow() {
    std::map<std::string, int> m{{"speed", 42}};

    // if with init-statement: `it` is scoped to the branch.
    if (auto it = m.find("speed"); it != m.end()) assert(it->second == 42);
    else assert(false);

    // Lifetime of the range expression IS extended.
    int sum = 0;
    for (int x : make_vector()) sum += x;
    assert(sum == 10);

    // `for (auto& x : make_holder().data)` DANGLES before C++23: the Holder
    // temporary dies, only the member reference was bound. The C++20 fix is an
    // init-statement that keeps the owner alive:
    int total = 0;
    for (auto h = make_holder(); int x : h.data) total += x;
    assert(total == 60);

    // switch: explicit fallthrough annotation, and braces to scope a case.
    auto classify = [](int code) {
        switch (code) {
            case 0: {
                const int local = 1;       // braces required to declare here
                return local;
            }
            case 1:
                [[fallthrough]];           // intentional, and documented
            case 2:
                return 2;
            default:
                return -1;
        }
    };
    assert(classify(0) == 1 && classify(1) == 2 && classify(9) == -1);
}

// -------------------------------------------------------------- namespaces
namespace tesla::autonomy {
struct Pose {
    double x{}, y{}, theta{};
};
// Found by ADL: it lives in the same namespace as Pose, so `swap(a, b)` and
// `std::cout << p` resolve without qualification.
inline void swap(Pose& a, Pose& b) noexcept {
    std::swap(a.x, b.x); std::swap(a.y, b.y); std::swap(a.theta, b.theta);
}
inline std::ostream& operator<<(std::ostream& os, const Pose& p) {
    return os << '(' << p.x << ", " << p.y << ", " << p.theta << ')';
}
}  // namespace tesla::autonomy

namespace {                     // anonymous namespace: internal linkage, and
int tu_local_counter = 0;       // unlike `static` it also works for types
struct TuLocalTag {};
int bump() { return ++tu_local_counter; }
}  // namespace

static void namespaces() {
    namespace ta = tesla::autonomy;                 // alias
    ta::Pose a{1, 2, 3}, b{4, 5, 6};

    using std::swap;                                // the correct swap idiom:
    swap(a, b);                                     // ADL finds ta::swap first
    assert(a.x == 4 && b.x == 1);

    std::cout << "  pose after swap: " << a << '\n';   // operator<< via ADL

    assert(bump() == 1 && bump() == 2);
    static_assert(std::is_class_v<TuLocalTag>);
}

// -------------------------------------------------------------- attributes
[[nodiscard]] static int must_check() { return 7; }
[[noreturn]] static void panic(const char* msg) {
    std::cerr << "panic: " << msg << '\n';
    std::abort();
}

static void attributes() {
    const int v = must_check();       // ignoring this would warn
    assert(v == 7);
    [[maybe_unused]] const int only_used_in_debug_builds = 1;
    if (false) panic("unreachable");  // [[noreturn]] lets the optimizer prune
}

int main() {
    enums();
    bitfields();
    type_punning();
    control_flow();
    namespaces();
    attributes();
    std::cout << "section 02: all checks passed\n";
}
