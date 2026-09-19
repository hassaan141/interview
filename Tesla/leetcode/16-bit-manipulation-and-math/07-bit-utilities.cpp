// PROBLEM     Not a LeetCode question -- the bit operations you will actually use in
//             embedded and systems C++, and which this interview may well ask about
//             directly.
// CONTENTS    alignment, power-of-two tests, ring-buffer index wrapping, register
//             field extraction and insertion, flag sets, endianness, and safe shifts.
// WHY         A foundations engineer writes this code. Being fluent here is worth more
//             than another DP problem.

#include <bit>
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

// ------------------------------------------------------------------ alignment
// Round UP to the next multiple of a POWER-OF-TWO alignment. This is what every
// allocator, DMA descriptor and packed-struct layout does.
constexpr std::uintptr_t align_up(std::uintptr_t value, std::uintptr_t alignment) {
    // (value + alignment - 1) & ~(alignment - 1)
    return (value + alignment - 1) & ~(alignment - 1);
}
constexpr std::uintptr_t align_down(std::uintptr_t value, std::uintptr_t alignment) {
    return value & ~(alignment - 1);
}
constexpr bool is_aligned(std::uintptr_t value, std::uintptr_t alignment) {
    return (value & (alignment - 1)) == 0;
}

// --------------------------------------------------------------- powers of two
constexpr bool is_power_of_two(std::uint64_t x) {
    return x != 0 && (x & (x - 1)) == 0;             // std::has_single_bit in C++20
}

// ------------------------------------------------------------- ring buffer wrap
// With a power-of-two capacity, the modulo is a single AND -- which is why every
// lock-free ring buffer requires it (course section 11).
constexpr std::size_t ring_index(std::size_t counter, std::size_t capacity_pow2) {
    return counter & (capacity_pow2 - 1);            // == counter % capacity
}

// --------------------------------------------------- register field get and set
// Extract `width` bits starting at `shift`, and insert a value into that field.
constexpr std::uint32_t field_get(std::uint32_t reg, unsigned shift, unsigned width) {
    return (reg >> shift) & ((std::uint32_t{1} << width) - 1);
}
constexpr std::uint32_t field_set(std::uint32_t reg, unsigned shift, unsigned width,
                                  std::uint32_t value) {
    const std::uint32_t mask = ((std::uint32_t{1} << width) - 1) << shift;
    return (reg & ~mask) | ((value << shift) & mask);   // clear then insert
}

// ------------------------------------------------------------------- flag sets
// A type-safe flag enum. enum class blocks accidental integer arithmetic, and the
// operators restore the bitwise vocabulary deliberately (course section 02).
enum class Status : std::uint8_t {
    None = 0, Ready = 1 << 0, Error = 1 << 1, Busy = 1 << 2, Calibrated = 1 << 3
};
// std::to_underlying is C++23; this spelling keeps the file at C++20.
constexpr auto raw(Status s) { return static_cast<std::underlying_type_t<Status>>(s); }
constexpr Status operator|(Status a, Status b) {
    return static_cast<Status>(raw(a) | raw(b));
}
constexpr Status operator&(Status a, Status b) {
    return static_cast<Status>(raw(a) & raw(b));
}
constexpr bool has_flag(Status set, Status flag) { return (set & flag) == flag; }

// ---------------------------------------------------------------- safe shifting
// Shifting by >= the bit width is UNDEFINED BEHAVIOR. A guarded helper is cheap.
template <typename T>
constexpr T safe_shl(T value, unsigned bits) {
    static_assert(std::is_unsigned_v<T>, "shift unsigned types only");
    return bits >= std::numeric_limits<T>::digits ? T{0} : static_cast<T>(value << bits);
}

int main() {
    // Alignment
    static_assert(align_up(0, 8) == 0);
    static_assert(align_up(1, 8) == 8);
    static_assert(align_up(8, 8) == 8);              // already aligned: unchanged
    static_assert(align_up(9, 64) == 64);
    static_assert(align_down(9, 8) == 8);
    static_assert(is_aligned(64, 64) && !is_aligned(65, 64));

    // Powers of two
    static_assert(is_power_of_two(1) && is_power_of_two(1024));
    static_assert(!is_power_of_two(0) && !is_power_of_two(3));
    static_assert(is_power_of_two(1) == std::has_single_bit(1u));
    static_assert(std::bit_ceil(100u) == 128u && std::bit_floor(100u) == 64u);

    // Ring buffer wrap: the AND matches the modulo for a power-of-two capacity.
    for (std::size_t i = 0; i < 40; ++i) assert(ring_index(i, 16) == i % 16);
    // And it keeps working across a counter wrap, which is why counters are unsigned.
    constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();
    assert(ring_index(kMax, 16) == 15);

    // Register fields: CAN-style id:11, rtr:1, dlc:4
    constexpr std::uint32_t reg = 0;
    constexpr std::uint32_t with_id = field_set(reg, 0, 11, 0x123);
    constexpr std::uint32_t with_dlc = field_set(with_id, 16, 4, 8);
    static_assert(field_get(with_dlc, 0, 11) == 0x123);
    static_assert(field_get(with_dlc, 16, 4) == 8);
    // Setting one field must not disturb another.
    static_assert(field_get(field_set(with_dlc, 16, 4, 3), 0, 11) == 0x123);
    // An oversized value is masked, not spilled into the neighbouring field.
    static_assert(field_get(field_set(with_dlc, 16, 4, 0xFF), 16, 4) == 0xF);

    // Flags
    constexpr Status state = Status::Ready | Status::Calibrated;
    static_assert(has_flag(state, Status::Ready));
    static_assert(has_flag(state, Status::Calibrated));
    static_assert(!has_flag(state, Status::Error));
    static_assert(sizeof(Status) == 1);              // fixed underlying type

    // Safe shifting: the UB case returns 0 instead of being undefined.
    static_assert(safe_shl(std::uint32_t{1}, 31) == 0x80000000u);
    static_assert(safe_shl(std::uint32_t{1}, 32) == 0);      // would be UB unguarded
    static_assert(safe_shl(std::uint8_t{1}, 9) == 0);

    // Endianness and rotation are DIFFERENT operations from bit reversal.
    static_assert(std::rotl(std::uint8_t{0b1000'0001}, 1) == 0b0000'0011);
    static_assert(std::rotr(std::uint8_t{0b0000'0011}, 1) == 0b1000'0001);
    static_assert(std::endian::native == std::endian::little ||
                  std::endian::native == std::endian::big);
    return 0;
}
