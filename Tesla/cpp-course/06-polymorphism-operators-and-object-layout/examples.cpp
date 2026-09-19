// 06 — Polymorphism, virtual dispatch, operator overloading, object layout.
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -g examples.cpp -o ex && ./ex
//   Look at the codegen too:
//     g++ -std=c++20 -O2 -S -masm=intel examples.cpp -o - | grep -A6 area

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <vector>

// -------------------------------------------------------- virtual dispatch
struct Shape {
    virtual ~Shape() = default;                       // REQUIRED: deleted via Shape*
    virtual double area() const = 0;                  // pure virtual -> abstract
    virtual std::string_view name() const { return "shape"; }
    double perimeter_estimate() const { return 4.0 * area(); }   // non-virtual,
};                                                    // calls a virtual: NVI-ish

struct Circle final : Shape {                         // final enables devirtualization
    double r{};
    explicit Circle(double radius) : r{radius} {}
    double area() const override { return 3.141592653589793 * r * r; }
    std::string_view name() const override { return "circle"; }
};

struct Square final : Shape {
    double s{};
    explicit Square(double side) : s{side} {}
    double area() const override { return s * s; }
    std::string_view name() const override { return "square"; }
};

static void virtual_dispatch() {
    // A polymorphic object carries a vptr: 8 bytes of pointer + the payload.
    static_assert(sizeof(Circle) == 16);              // vptr(8) + double(8)
    static_assert(sizeof(Square) == 16);
    static_assert(!std::is_trivially_copyable_v<Circle>);   // the vptr is ours, not yours
    static_assert(!std::is_standard_layout_v<Circle>);

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(1.0));
    shapes.push_back(std::make_unique<Square>(2.0));

    double total = 0.0;
    for (const auto& s : shapes) total += s->area();   // indirect call, not inlinable
    assert(total > 7.14 && total < 7.15);
    assert(shapes[0]->name() == "circle");

    // RTTI: typeid on a polymorphic glvalue reports the DYNAMIC type.
    assert(typeid(*shapes[0]) == typeid(Circle));
    assert(dynamic_cast<Square*>(shapes[0].get()) == nullptr);   // safely reports "no"
    assert(dynamic_cast<Circle*>(shapes[0].get()) != nullptr);

    // Devirtualized: the dynamic type of a local object is known, so this call
    // inlines completely at -O2. `final` extends that to calls through pointers.
    Circle local{2.0};
    assert(local.area() > 12.56 && local.area() < 12.57);
}

// ------------------------------------------------------- override and hiding
struct B {
    virtual ~B() = default;
    virtual void f(int) const {}
    void g(int)    {}
    void g(double) {}
};
struct DBad : B {
    // void f(int) {}          // NOT an override (missing const): `override` would
                               // have made this a compile error. Silent bug.
    void f(int) const override {}     // correct
};
struct DHides : B {
    void g(int) {}             // hides BOTH B::g overloads
};
struct DUnhides : B {
    using B::g;                // brings the base overload set back in
    void g(int) {}
};

static void overriding_and_hiding() {
    DHides   h;
    DUnhides u;
    // h.g(1.0) would call DHides::g(int) after a double->int conversion.
    // u.g(1.0) correctly calls B::g(double).
    h.g(1.0);
    u.g(1.0);
    static_assert(std::is_base_of_v<B, DUnhides>);
}

// --------------------------------------------------------- operator overloading
class Vec3 {
    double v_[3]{};
public:
    constexpr Vec3() = default;
    constexpr Vec3(double x, double y, double z) : v_{x, y, z} {}

    constexpr double  operator[](std::size_t i) const { return v_[i]; }
    constexpr double& operator[](std::size_t i)       { return v_[i]; }

    constexpr Vec3& operator+=(const Vec3& o) noexcept {
        for (int i = 0; i < 3; ++i) v_[i] += o.v_[i];
        return *this;
    }
    constexpr Vec3& operator*=(double s) noexcept {
        for (auto& c : v_) c *= s;
        return *this;
    }
    constexpr Vec3 operator-() const noexcept { return Vec3{-v_[0], -v_[1], -v_[2]}; }

    // Hidden friends: only findable by ADL, symmetric in their conversions, and
    // each is one line because it reuses the compound-assignment member.
    friend constexpr Vec3 operator+(Vec3 a, const Vec3& b) noexcept { return a += b; }
    friend constexpr Vec3 operator*(Vec3 a, double s) noexcept      { return a *= s; }
    friend constexpr Vec3 operator*(double s, Vec3 a) noexcept      { return a *= s; }
    friend constexpr double dot(const Vec3& a, const Vec3& b) noexcept {
        return a.v_[0]*b.v_[0] + a.v_[1]*b.v_[1] + a.v_[2]*b.v_[2];
    }
    // C++20: these two give you !=, <, >, <=, >= and the reversed forms for free.
    friend constexpr bool operator==(const Vec3&, const Vec3&) = default;
    friend constexpr auto operator<=>(const Vec3&, const Vec3&) = default;

    friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        return os << '(' << v[0] << ", " << v[1] << ", " << v[2] << ')';
    }
    explicit constexpr operator bool() const noexcept {
        return v_[0] != 0.0 || v_[1] != 0.0 || v_[2] != 0.0;
    }
};

static void operators() {
    constexpr Vec3 a{1, 2, 3};
    constexpr Vec3 b{4, 5, 6};
    static_assert(dot(a, b) == 32.0);
    static_assert((a + b)[0] == 5.0);
    static_assert((2.0 * a)[2] == 6.0);       // scalar on the LEFT: needs a non-member
    static_assert((-a)[0] == -1.0);
    static_assert(a == Vec3{1, 2, 3});
    static_assert(a != b);                    // synthesized from operator==
    static_assert(a < b);                     // synthesized from operator<=>
    static_assert(!(a > b));
    static_assert(static_cast<bool>(a));
    static_assert(!static_cast<bool>(Vec3{}));
    std::cout << "  vec: " << a + b << '\n';

    // Prefix returns a reference to *this; postfix returns the old value by copy.
    struct Counter {
        int n{};
        Counter& operator++()    { ++n; return *this; }
        Counter  operator++(int) { Counter old = *this; ++n; return old; }
    };
    Counter c;
    assert((++c).n == 1);
    assert((c++).n == 1 && c.n == 2);         // postfix had to copy
}

// -------------------------------------------------------------- object layout
struct Aggregate      { int a; double b; };
struct StandardLayout { private: int a_; int b_; public: StandardLayout():a_{},b_{}{} };
struct MixedAccess    { public: int a; private: int b; };
struct Polymorphic    { virtual ~Polymorphic() = default; int a; };

struct Empty {};
struct WithEmptyBase   : Empty { int x; };
struct WithEmptyMember { Empty e; int x; };
struct WithNoUniqueAddr { [[no_unique_address]] Empty e; int x; };

struct BadOrder  { char a; double b; char c; };
struct GoodOrder { double b; char a; char c; };

static void object_layout() {
    static_assert(std::is_aggregate_v<Aggregate>);
    static_assert(std::is_trivially_copyable_v<Aggregate>);
    static_assert(std::is_standard_layout_v<Aggregate>);
    static_assert(offsetof(Aggregate, b) == 8);          // defined for std layout

    static_assert(std::is_standard_layout_v<StandardLayout>);   // one access group
    static_assert(!std::is_standard_layout_v<MixedAccess>);     // two access groups
    static_assert(!std::is_standard_layout_v<Polymorphic>);     // has a vptr
    static_assert(!std::is_trivially_copyable_v<Polymorphic>);

    // Empty base optimization, and the C++20 fix for empty MEMBERS.
    static_assert(sizeof(Empty) == 1);
    static_assert(sizeof(WithEmptyBase) == 4);
    static_assert(sizeof(WithEmptyMember) == 8);
    static_assert(sizeof(WithNoUniqueAddr) == 4);

    // Member order changes the size by 50% here.
    static_assert(sizeof(BadOrder) == 24);
    static_assert(sizeof(GoodOrder) == 16);
    static_assert(alignof(GoodOrder) == 8);

    // This is the property that lets you memcpy a frame into shared memory.
    Aggregate src{7, 1.5};
    std::byte raw[sizeof(Aggregate)];
    std::memcpy(raw, &src, sizeof src);
    Aggregate dst{};
    std::memcpy(&dst, raw, sizeof dst);
    assert(dst.a == 7 && dst.b == 1.5);
}

// ------------------------------------- the closed-set alternative to virtual
// Same behaviour, no vptr, no allocation, contiguous storage, and the compiler
// can see every case.
using AnyShape = std::variant<Circle, Square>;

static double area_of(const AnyShape& s) {
    return std::visit([](const auto& concrete) { return concrete.area(); }, s);
}

static void variant_polymorphism() {
    std::vector<AnyShape> shapes{Circle{1.0}, Square{2.0}};   // no heap per element
    double total = 0.0;
    for (const auto& s : shapes) total += area_of(s);
    assert(total > 7.14 && total < 7.15);
    // Storage is inline: max(sizeof(Circle), sizeof(Square)) + a tag.
    static_assert(sizeof(AnyShape) <= 32);
}

int main() {
    virtual_dispatch();
    overriding_and_hiding();
    operators();
    object_layout();
    variant_polymorphism();
    std::cout << "section 06: all checks passed\n";
}
