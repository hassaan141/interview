// 13 — Debugging and testing, demonstrated on a real unit under test.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex
//   g++ -std=c++20 -g -fsanitize=address,undefined examples.cpp -o ex && ./ex
//
// The unit under test is a frame parser -- exactly the kind of thing a foundations
// team owns, and exactly the kind of thing that must never crash on bad input.
//
// See also ./sanitizer_lab/run.sh (each sanitizer catching its own bug class)
// and ./CMakeLists.txt (the build/test wiring).

#include "minitest.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <numeric>
#include <type_traits>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================ unit under test
struct Frame {
    std::uint16_t id{};
    std::uint8_t  payload_len{};
    std::uint8_t  payload[8]{};

    friend bool operator==(const Frame&, const Frame&) = default;
};

enum class ParseError : std::uint8_t { TooShort, BadLength, BadChecksum };

// Wire format: [id_lo][id_hi][len][payload...len bytes][checksum]
// checksum = XOR of every preceding byte.
class Parser {
public:
    // Total bytes a frame with `len` payload bytes occupies on the wire.
    static constexpr std::size_t wire_size(std::uint8_t len) noexcept {
        return 4u + static_cast<std::size_t>(len);
    }
    static constexpr std::uint8_t kMaxPayload = 8;

    // Never throws, never reads out of bounds, never allocates.
    static std::optional<Frame> parse(std::span<const std::uint8_t> in,
                                      ParseError* err = nullptr) noexcept {
        auto fail = [err](ParseError e) -> std::optional<Frame> {
            if (err) *err = e;
            return std::nullopt;
        };
        if (in.size() < 4) return fail(ParseError::TooShort);      // id+len+checksum
        const std::uint8_t len = in[2];
        if (len > kMaxPayload)          return fail(ParseError::BadLength);
        if (in.size() < wire_size(len)) return fail(ParseError::TooShort);

        std::uint8_t sum = 0;
        for (std::size_t i = 0; i + 1 < wire_size(len); ++i) sum ^= in[i];
        if (sum != in[wire_size(len) - 1]) return fail(ParseError::BadChecksum);

        Frame f{};
        f.id = static_cast<std::uint16_t>(in[0] | (in[1] << 8));
        f.payload_len = len;
        std::copy_n(in.begin() + 3, len, f.payload);
        return f;
    }

    // The inverse, so we can property-test round-tripping.
    static std::vector<std::uint8_t> serialize(const Frame& f) {
        std::vector<std::uint8_t> out;
        out.reserve(wire_size(f.payload_len));
        out.push_back(static_cast<std::uint8_t>(f.id & 0xFF));
        out.push_back(static_cast<std::uint8_t>(f.id >> 8));
        out.push_back(f.payload_len);
        out.insert(out.end(), f.payload, f.payload + f.payload_len);
        out.push_back(std::accumulate(out.begin(), out.end(), std::uint8_t{0},
                                      std::bit_xor<std::uint8_t>{}));
        return out;
    }
};

// A clock-injected component, so its tests need no sleep() and are not flaky.
template <typename Clock>
class RateLimiter {
    typename Clock::duration  period_;
    typename Clock::time_point next_{};
public:
    explicit RateLimiter(typename Clock::duration period) : period_{period} {}
    bool allow() {
        const auto now = Clock::now();
        if (now < next_) return false;
        next_ = now + period_;
        return true;
    }
};

// The fake clock: tests control time explicitly. No sleeps, no flakes.
struct FakeClock {
    using duration   = std::chrono::milliseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<FakeClock, duration>;
    static constexpr bool is_steady = true;
    static time_point current;
    static time_point now() noexcept { return current; }
    static void advance(duration d) { current += d; }
};
FakeClock::time_point FakeClock::current{};

// ===================================================================== tests
// Compile-time tests are free and cannot regress silently.
static_assert(Parser::wire_size(0) == 4);
static_assert(Parser::wire_size(8) == 12);
static_assert(sizeof(Frame) == 12, "pin the layout so a change is a build failure");
static_assert(std::is_trivially_copyable_v<Frame>);

TEST(Parser, RoundTripsAValidFrame) {
    Frame in{};
    in.id = 0x1234;
    in.payload_len = 3;
    in.payload[0] = 0xAA; in.payload[1] = 0xBB; in.payload[2] = 0xCC;

    const auto bytes = Parser::serialize(in);
    const auto out = Parser::parse(bytes);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(*out, in);
}

// Boundary cases: empty, one below the minimum, exactly the minimum, exactly the
// maximum, one above the maximum. This is where the bugs live.
TEST(Parser, RejectsEmptyInput) {
    ParseError e{};
    EXPECT_FALSE(Parser::parse({}, &e).has_value());
    EXPECT_TRUE(e == ParseError::TooShort);
}

TEST(Parser, RejectsOneByteBelowMinimum) {
    const std::uint8_t bytes[]{0x34, 0x12, 0x00};      // 3 bytes, minimum is 4
    ParseError e{};
    EXPECT_FALSE(Parser::parse(bytes, &e).has_value());
    EXPECT_TRUE(e == ParseError::TooShort);
}

TEST(Parser, AcceptsExactlyTheMinimum) {
    Frame f{};
    f.id = 1;
    f.payload_len = 0;
    const auto bytes = Parser::serialize(f);
    EXPECT_EQ(bytes.size(), 4u);
    EXPECT_TRUE(Parser::parse(bytes).has_value());
}

TEST(Parser, AcceptsTheMaximumPayload) {
    Frame f{};
    f.payload_len = Parser::kMaxPayload;
    for (std::uint8_t i = 0; i < f.payload_len; ++i) f.payload[i] = i;
    EXPECT_TRUE(Parser::parse(Parser::serialize(f)).has_value());
}

TEST(Parser, RejectsAnOversizedLengthField) {
    // A hostile length field must not make the parser read past the buffer --
    // this is the test that ASan would otherwise catch for you in production.
    const std::uint8_t bytes[]{0x00, 0x00, 0xFF, 0x00};   // len = 255
    ParseError e{};
    EXPECT_FALSE(Parser::parse(bytes, &e).has_value());
    EXPECT_TRUE(e == ParseError::BadLength);
}

TEST(Parser, RejectsATruncatedPayload) {
    const std::uint8_t bytes[]{0x00, 0x00, 0x08, 0x01, 0x02};  // claims 8, has 2
    ParseError e{};
    EXPECT_FALSE(Parser::parse(bytes, &e).has_value());
    EXPECT_TRUE(e == ParseError::TooShort);
}

TEST(Parser, RejectsABadChecksum) {
    Frame f{};
    f.id = 7; f.payload_len = 1; f.payload[0] = 0x55;
    auto bytes = Parser::serialize(f);
    bytes.back() ^= 0xFF;                                  // corrupt the checksum
    ParseError e{};
    EXPECT_FALSE(Parser::parse(bytes, &e).has_value());
    EXPECT_TRUE(e == ParseError::BadChecksum);
}

// A property test: for ANY valid frame, serialize->parse is the identity.
// Seeded, so a failure is reproducible -- an unseeded random test is a flaky test.
TEST(Parser, PropertyRoundTripForRandomFrames) {
    std::mt19937 gen{0xC0FFEE};
    std::uniform_int_distribution<int> byte{0, 255};
    std::uniform_int_distribution<int> len{0, Parser::kMaxPayload};
    for (int iter = 0; iter < 2000; ++iter) {
        Frame f{};
        f.id = static_cast<std::uint16_t>(byte(gen) | (byte(gen) << 8));
        f.payload_len = static_cast<std::uint8_t>(len(gen));
        for (std::uint8_t i = 0; i < f.payload_len; ++i)
            f.payload[i] = static_cast<std::uint8_t>(byte(gen));
        const auto out = Parser::parse(Parser::serialize(f));
        ASSERT_TRUE(out.has_value());
        EXPECT_EQ(*out, f);
    }
}

// A fuzz-style test: random garbage must never crash, hang, or read out of bounds.
// Run this file under ASan and this test becomes a real memory-safety check.
TEST(Parser, NeverCrashesOnArbitraryInput) {
    std::mt19937 gen{12345};
    std::uniform_int_distribution<int> byte{0, 255};
    std::uniform_int_distribution<std::size_t> size{0, 32};
    for (int iter = 0; iter < 20000; ++iter) {
        std::vector<std::uint8_t> junk(size(gen));
        for (auto& b : junk) b = static_cast<std::uint8_t>(byte(gen));
        (void)Parser::parse(junk);        // the assertion is "does not crash"
    }
    EXPECT_TRUE(true);
}

// Floating point: never EXPECT_EQ.
TEST(Numerics, UsesToleranceNotEquality) {
    const double sum = 0.1 + 0.2;
    // EXPECT_EQ(sum, 0.3);   <- would FAIL, and the test would be the bug
    EXPECT_NEAR(sum, 0.3, 1e-12);
}

// Time-dependent code, tested without a single sleep().
TEST(RateLimiter, EnforcesThePeriodWithoutSleeping) {
    using namespace std::chrono_literals;
    FakeClock::current = FakeClock::time_point{};
    RateLimiter<FakeClock> limiter{100ms};

    EXPECT_TRUE(limiter.allow());          // first call always passes
    EXPECT_FALSE(limiter.allow());         // immediately after: blocked
    FakeClock::advance(99ms);
    EXPECT_FALSE(limiter.allow());         // one millisecond short
    FakeClock::advance(1ms);
    EXPECT_TRUE(limiter.allow());          // exactly at the boundary
}

TEST(Contracts, ThrowsOnAViolatedPrecondition) {
    auto at = [](std::span<const int> s, std::size_t i) {
        if (i >= s.size()) throw std::out_of_range{"index"};
        return s[i];
    };
    const int data[]{1, 2, 3};
    EXPECT_EQ(at(data, 2), 3);
    EXPECT_THROW(at(data, 3), std::out_of_range);
}

int main() {
    std::printf("section 13: running tests\n");
    const int rc = minitest::run_all();
    std::printf(rc == 0 ? "section 13: all checks passed\n"
                        : "section 13: FAILURES (see above)\n");
    return rc;
}
