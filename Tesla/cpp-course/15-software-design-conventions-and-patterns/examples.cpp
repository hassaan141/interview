// 15 — Software design: RAII, PIMPL, CRTP, type erasure, strong types, DI.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// ======================================================= strong types
// Make interfaces hard to misuse: a unit bug becomes a COMPILE error.
template <typename Tag, typename Rep = double>
class Quantity {
    Rep v_{};
public:
    constexpr Quantity() = default;
    constexpr explicit Quantity(Rep v) : v_{v} {}           // explicit: no accidents
    constexpr Rep value() const noexcept { return v_; }

    constexpr Quantity& operator+=(Quantity o) { v_ += o.v_; return *this; }
    friend constexpr Quantity operator+(Quantity a, Quantity b) { return a += b; }
    friend constexpr Quantity operator*(Quantity a, Rep s) { return Quantity{a.v_ * s}; }
    friend constexpr auto operator<=>(Quantity, Quantity) = default;
    friend constexpr bool operator==(Quantity, Quantity) = default;
};
struct MetersTag {};
struct SecondsTag {};
struct MpsTag {};
using Meters  = Quantity<MetersTag>;
using Seconds = Quantity<SecondsTag>;
using Mps     = Quantity<MpsTag>;

constexpr Mps operator/(Meters d, Seconds t) { return Mps{d.value() / t.value()}; }

static void strong_types() {
    constexpr Meters  d{100.0};
    constexpr Seconds t{4.0};
    constexpr Mps     v = d / t;
    static_assert(v == Mps{25.0});

    // These are all COMPILE errors, which is the entire point:
    //   Meters bad = d + t;        // cannot add metres to seconds
    //   Mps    bad2 = d;           // no implicit conversion
    //   set_speed(30);             // if set_speed takes Mps
    static_assert(d + Meters{50.0} == Meters{150.0});
    static_assert(sizeof(Meters) == sizeof(double));   // zero runtime cost
}

// Prefer an enum over a bool parameter: the call site becomes readable.
enum class Blocking : bool { No = false, Yes = true };
enum class Retry    : bool { No = false, Yes = true };
static int connect(Blocking blocking, Retry retry) {
    return (blocking == Blocking::Yes ? 1 : 0) + (retry == Retry::Yes ? 2 : 0);
}

static void no_bool_parameters() {
    // connect(true, false);  <- unreadable, and easy to transpose
    assert(connect(Blocking::Yes, Retry::No) == 1);
    assert(connect(Blocking::No, Retry::Yes) == 2);
}

// ============================================================ PIMPL
// The public header would contain ONLY this: no implementation details, no heavy
// includes, and a layout that never changes -> ABI stable and fast to compile.
class Detector {
public:
    Detector();
    ~Detector();                                   // OUT OF LINE: Impl is incomplete
    Detector(Detector&&) noexcept;                 // also out of line
    Detector& operator=(Detector&&) noexcept;
    Detector(const Detector&) = delete;
    Detector& operator=(const Detector&) = delete;

    std::size_t detect(std::span<const float> samples, float threshold);
    std::size_t total_detections() const noexcept;
private:
    struct Impl;                                   // declared, not defined
    std::unique_ptr<Impl> impl_;
};

// ---- everything below would live in detector.cpp --------------------------
struct Detector::Impl {
    std::size_t total{0};
    std::vector<float> scratch;                    // a heavy member, invisible to users
};
Detector::Detector() : impl_{std::make_unique<Impl>()} {}
Detector::~Detector() = default;                   // defined where Impl is complete
Detector::Detector(Detector&&) noexcept = default;
Detector& Detector::operator=(Detector&&) noexcept = default;

std::size_t Detector::detect(std::span<const float> samples, float threshold) {
    impl_->scratch.assign(samples.begin(), samples.end());
    std::size_t n = 0;
    for (float s : impl_->scratch) n += (s > threshold);
    impl_->total += n;
    return n;
}
std::size_t Detector::total_detections() const noexcept { return impl_->total; }
// --------------------------------------------------------------------------

static void pimpl() {
    Detector d;
    const float samples[]{0.1f, 0.9f, 0.5f, 0.99f};
    assert(d.detect(samples, 0.6f) == 2);
    assert(d.detect(samples, 0.4f) == 3);
    assert(d.total_detections() == 5);

    Detector moved = std::move(d);                 // move works, copy is deleted
    assert(moved.total_detections() == 5);
    static_assert(sizeof(Detector) == sizeof(void*));   // the ABI-stable part
}

// ============================================================ CRTP
// Static polymorphism: no vptr, fully inlinable, dispatch resolved at compile time.
template <typename Derived>
class ShapeBase {
public:
    constexpr double area() const {
        return static_cast<const Derived&>(*this).area_impl();
    }
    // Shared, non-virtual behaviour written ONCE against the derived interface:
    constexpr bool bigger_than(double threshold) const { return area() > threshold; }
};

class CrtpCircle : public ShapeBase<CrtpCircle> {
    double r_{};
public:
    constexpr explicit CrtpCircle(double r) : r_{r} {}
    constexpr double area_impl() const { return 3.141592653589793 * r_ * r_; }
};
class CrtpSquare : public ShapeBase<CrtpSquare> {
    double s_{};
public:
    constexpr explicit CrtpSquare(double s) : s_{s} {}
    constexpr double area_impl() const { return s_ * s_; }
};

static void crtp() {
    constexpr CrtpCircle c{1.0};
    constexpr CrtpSquare s{2.0};
    static_assert(s.area() == 4.0);                       // compile-time dispatch!
    static_assert(s.bigger_than(3.0));
    static_assert(sizeof(CrtpCircle) == sizeof(double));  // NO vptr
    static_assert(!std::is_polymorphic_v<CrtpCircle>);
    assert(c.area() > 3.14 && c.area() < 3.15);

    // The cost: no common base type, so you CANNOT do
    //   std::vector<ShapeBase*> mixed;
    // For a heterogeneous container you need a variant or virtual dispatch.
    using AnyShape = std::variant<CrtpCircle, CrtpSquare>;
    const std::vector<AnyShape> shapes{CrtpCircle{1.0}, CrtpSquare{2.0}};
    double total = 0.0;
    for (const auto& sh : shapes)
        total += std::visit([](const auto& x) { return x.area(); }, sh);
    assert(total > 7.14 && total < 7.15);
}

// ================================================= type erasure (value semantics
// over an open set of types, without inheritance leaking into the interface)
class AnyScorer {
    struct Concept {
        virtual ~Concept() = default;
        virtual double score(double x) const = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
    };
    template <typename T>
    struct Model final : Concept {
        T impl;
        explicit Model(T t) : impl{std::move(t)} {}
        double score(double x) const override { return impl(x); }
        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(impl);
        }
    };
    std::unique_ptr<Concept> self_;
public:
    template <typename T>
        requires (!std::same_as<std::remove_cvref_t<T>, AnyScorer>)
    AnyScorer(T&& t) : self_{std::make_unique<Model<std::decay_t<T>>>(std::forward<T>(t))} {}

    AnyScorer(const AnyScorer& o) : self_{o.self_->clone()} {}   // VALUE semantics
    AnyScorer(AnyScorer&&) noexcept = default;
    AnyScorer& operator=(AnyScorer o) noexcept { self_.swap(o.self_); return *this; }

    double score(double x) const { return self_->score(x); }
};

struct Linear { double k; double operator()(double x) const { return k * x; } };

static void type_erasure() {
    // Any callable works; none of them inherit from anything.
    std::vector<AnyScorer> scorers;
    scorers.emplace_back(Linear{2.0});
    scorers.emplace_back([](double x) { return x * x; });
    scorers.emplace_back([](double x) { return std::sqrt(x); });

    assert(scorers[0].score(3.0) == 6.0);
    assert(scorers[1].score(3.0) == 9.0);

    AnyScorer copied = scorers[0];              // copyable, unlike an interface pointer
    assert(copied.score(4.0) == 8.0);
    // This is how std::function, std::any and std::pmr::memory_resource are built,
    // and it is the answer to "value semantics over an open set of types".
}

// ============================================ dependency injection vs. singleton
// The singleton (shown to be criticized, not copied):
class ConfigSingleton {
public:
    static ConfigSingleton& instance() { static ConfigSingleton c; return c; }
    int level() const noexcept { return level_; }
    void set_level(int l) noexcept { level_ = l; }
    ConfigSingleton(const ConfigSingleton&) = delete;
    ConfigSingleton& operator=(const ConfigSingleton&) = delete;
private:
    ConfigSingleton() = default;
    int level_{3};
};

// Dependency injection: the same single instance, but an EXPLICIT, substitutable
// dependency -- which is what makes the component unit-testable.
struct IClock {
    virtual ~IClock() = default;
    virtual std::int64_t now_ms() const = 0;
};
struct SystemClock final : IClock {
    std::int64_t now_ms() const override {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
struct FakeClock final : IClock {                  // the test double
    std::int64_t t{0};
    std::int64_t now_ms() const override { return t; }
};

class Watchdog {
    const IClock& clock_;                          // injected, not constructed
    std::int64_t timeout_ms_;
    std::int64_t last_kick_;
public:
    Watchdog(const IClock& clock, std::int64_t timeout_ms)
        : clock_{clock}, timeout_ms_{timeout_ms}, last_kick_{clock.now_ms()} {}
    void kick() { last_kick_ = clock_.now_ms(); }
    bool expired() const { return clock_.now_ms() - last_kick_ > timeout_ms_; }
};

static void dependency_injection() {
    FakeClock clock;                               // full control over time
    Watchdog wd{clock, 100};
    assert(!wd.expired());
    clock.t = 100;
    assert(!wd.expired());                         // exactly at the boundary
    clock.t = 101;
    assert(wd.expired());
    wd.kick();
    assert(!wd.expired());
    // With ConfigSingleton::instance() inside Watchdog, none of this is testable
    // without global mutation, and the tests could not run in parallel.
    assert(ConfigSingleton::instance().level() == 3);   // shown for contrast only
}

// ===================================== NVI: enforce a contract on subclasses
class Stage {
public:
    virtual ~Stage() = default;
    // PUBLIC and NON-virtual: the invariant checks live here and cannot be skipped.
    std::size_t run(std::span<const float> in) {
        assert(!in.empty() && "Stage::run requires a non-empty input");
        const std::size_t produced = run_impl(in);
        assert(produced <= in.size() && "a stage may not produce more than it consumes");
        ++invocations_;
        return produced;
    }
    std::size_t invocations() const noexcept { return invocations_; }
private:
    virtual std::size_t run_impl(std::span<const float> in) = 0;   // the hook
    std::size_t invocations_{0};
};

class ThresholdStage final : public Stage {
    float t_;
    std::size_t run_impl(std::span<const float> in) override {
        std::size_t n = 0;
        for (float v : in) n += (v > t_);
        return n;
    }
public:
    explicit ThresholdStage(float t) : t_{t} {}
};

static void non_virtual_interface() {
    ThresholdStage s{0.5f};
    const float data[]{0.1f, 0.6f, 0.9f};
    Stage& base = s;
    assert(base.run(data) == 2);
    assert(base.run(data) == 2);
    assert(base.invocations() == 2);       // bookkeeping the subclass cannot forget
}

int main() {
    strong_types();
    no_bool_parameters();
    pimpl();
    crtp();
    type_erasure();
    dependency_injection();
    non_virtual_interface();
    std::cout << "section 15: all checks passed\n";
}
