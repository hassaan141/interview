# 03 — Memory: Pointers, References, Stack/Heap, Initialization, `const`/`constexpr`, Casts, Alignment

Course chapter: **7 (Basic Concepts V)**

**This is the highest-yield section in the repo.** Every C++ interview at a
systems company lives here. If you only have one day, spend it here.

---

## 1. Pointers

```cpp
int  x = 42;
int* p = &x;          // address-of
*p = 7;               // dereference
p->member;            // == (*p).member
```

### Pointer arithmetic
`p + n` advances by `n * sizeof(*p)` **bytes**. It is only defined within a
single array object (plus one past the end). Subtraction gives `std::ptrdiff_t`.

```cpp
int a[5]{};
int* q = a + 5;       // OK: one-past-the-end, may NOT be dereferenced
int* r = a + 6;       // UB just to FORM this pointer
a[3] == *(a + 3) == *(3 + a) == 3[a];   // all the same; the last one is legal C++
```

`void*` can point at anything but cannot be dereferenced or arithmetic'd
(gcc extension allows `void*` arithmetic as `char*` — don't rely on it). C++ does
*implicit* conversion `T* → void*` but requires `static_cast` back.

### Wild vs. dangling vs. null
- **wild**: never initialized. `int* p;` then `*p`. Always initialize (`= nullptr`).
- **dangling**: pointed-to object's lifetime ended (freed, went out of scope,
  vector reallocated). Use-after-free.
- **null**: `nullptr`. Dereferencing is UB. `nullptr` has type
  `std::nullptr_t`, which is why it beats `NULL`/`0` in overload resolution:

```cpp
void f(int);
void f(char*);
f(NULL);      // ambiguous or picks f(int) — NULL is 0
f(nullptr);   // unambiguously f(char*)
```

## 2. References

A reference is an alias. It **must** be initialized, **cannot** be rebound,
cannot be null (without UB), and has no size of its own.

| | pointer | reference |
| --- | --- | --- |
| can be null | yes | no (legally) |
| can be reseated | yes | no |
| needs init | no (but should) | yes |
| arithmetic | yes | no |
| `sizeof` | pointer size | size of the referent |
| array of them | yes | **no** |

Lifetime extension — know the exact rule:
```cpp
const std::string& r = std::string("hi");   // temporary's lifetime extended to r's
std::string&& rr = std::string("hi");       // also extended
const std::string& bad = f().substr(0,2);   // extended (the substr temporary)
const int& worse = Widget{}.member;         // NOT extended past the full-expr
                                            // in a return statement -> dangles
```
The rule: binding a **temporary** directly to a reference extends its lifetime to
that of the reference — **except** when the reference is a function parameter
(ends at the full-expression), a return value (dangles), or a member initialized
in a ctor-init-list (ends at the end of the ctor). Those three exceptions are the
interview question.

## 3. Stack vs. heap

| | stack | heap (free store) |
| --- | --- | --- |
| allocation | bump a register (`sub rsp, N`) — ~1 instruction | `malloc`: free-list walk, possibly a syscall, possibly a lock |
| size | fixed, small (Linux default 8 MB, threads often 512 KB-2 MB) | large, limited by RAM/`ulimit` |
| lifetime | automatic, LIFO, exception-safe | manual (or RAII) |
| locality | excellent, hot in L1 | depends, often a cache miss |
| failure | stack overflow (no error, just a crash / guard-page SIGSEGV) | `std::bad_alloc` or `nullptr` |

**For a real-time autonomy loop: do not allocate in the hot path.** Preallocate,
`reserve()`, use arenas/pools, or fixed-capacity containers. Be ready to say
this, because it is the single most important performance opinion for this role.

```cpp
int* p  = new int(3);        // allocate + construct
delete p;                    // destruct + deallocate
int* a  = new int[10]{};     // array form
delete[] a;                  // MUST match — `delete a` is UB
// nothrow:
int* n = new (std::nothrow) int;     // returns nullptr instead of throwing
// placement new: construct in memory you already own
alignas(Widget) std::byte buf[sizeof(Widget)];
Widget* w = new (buf) Widget{};      // no allocation
w->~Widget();                        // must destroy manually
```

Rules: mismatched `new`/`delete[]` is UB; `delete` on a non-`new` pointer is UB;
double delete is UB; `delete nullptr` is **fine** (no-op). In modern C++ you
should almost never write `new`/`delete` — use `std::make_unique`,
`std::make_shared`, or a container (see section 10).

Memory leak vs. dangling: a leak is memory you can no longer reach; a dangling
pointer is a reachable pointer to memory you no longer own. Leaks waste; dangling
corrupts. Dangling is far worse.

## 4. Initialization — the part that actually confuses people

```cpp
int a;             // default-init: INDETERMINATE for built-ins at block scope
                   // (zero for static/thread storage duration)
int b{};           // value-init -> 0
int c = 5;         // copy-init
int d{5};          // direct-list-init (no narrowing allowed!)
int e(5);          // direct-init
auto f = int{5};

struct S { int x; int y = 2; };
S s1;              // x indeterminate, y == 2
S s2{};            // aggregate-init: x == 0, y == 2
S s3{1, 2};        // aggregate-init
S s4{.x = 1};      // designated init (C++20), y == 2

std::vector<int> v1(3, 0);   // 3 elements, value 0     <-- parentheses
std::vector<int> v2{3, 0};   // 2 elements: {3, 0}       <-- braces!
```

That last pair is the most-asked initialization question in existence.
`std::initializer_list` wins overload resolution over everything else when
braces are used.

The "most vexing parse":
```cpp
Widget w();        // declares a FUNCTION returning Widget, not an object!
Widget w{};        // an object
Widget w2(Foo());  // function taking a pointer-to-function-returning-Foo
Widget w2{Foo{}};  // an object
```

Braced init also **bans narrowing**, which is a free bug filter:
```cpp
int  i{3.5};        // ERROR (good)
int  j = 3.5;       // 3, silent truncation
char k{300};        // ERROR
```

Rule to state: **prefer `{}` everywhere except when you mean a container size.**

### Structured bindings (C++17)
```cpp
auto [a, b] = std::pair{1, 2.0};                   // copies
auto& [x, y] = some_struct;                        // references into it
const auto& [k, v] = *map.begin();                 // no copy
auto [q, r] = std::div(17, 5);                     // works on arrays, tuples,
                                                   // and public-only aggregates
```

## 5. `const`, `constexpr`, `consteval`, `constinit`

```cpp
const int  n = 5;                  // runtime const (but usable as constant expr here)
constexpr int m = 5;               // guaranteed compile-time constant

int x = 1;
const int*       p1 = &x;          // pointer to const int   (can't write *p1)
int* const       p2 = &x;          // const pointer to int    (can't reseat p2)
const int* const p3 = &x;          // both
// read right-to-left. `const int*` == `int const*`.
```

`constexpr` **function**: *may* be evaluated at compile time if its arguments
are constant expressions; otherwise it runs at runtime. `consteval` function
(C++20): **must** be evaluated at compile time. `constinit` (C++20): a variable
with static storage duration that must be constant-**initialized** (kills the
static-initialization-order fiasco) but is still mutable.

```cpp
constexpr int square(int v) { return v * v; }
static_assert(square(4) == 16);          // compile time
int r = square(read_input());            // runtime, same function

consteval int must_be_ct(int v) { return v + 1; }
constexpr int k = must_be_ct(1);         // OK
// int bad = must_be_ct(runtime_value);  // ERROR

constinit int counter = square(3);       // zero cost at startup, mutable after

if constexpr (sizeof(void*) == 8) { /* the other branch isn't even compiled */ }
if consteval { /* C++23: are we in a constant evaluation? */ }
// C++20: std::is_constant_evaluated()
```

`constexpr` on a variable also implies `const`. `constexpr` on a member function
does **not** imply `const` (since C++14).

`volatile` means "this object may change outside the program's control" — it
suppresses caching of the value in a register and prevents reordering *of that
access* by the compiler. It is for **memory-mapped hardware registers and signal
handlers**. It is **not** a threading primitive — it gives you no atomicity and
no inter-thread ordering. Saying this correctly is a strong signal.

## 6. Explicit casts — all five, and when

```cpp
static_cast<T>(e)        // related types: numeric conversions, up/down the
                         // hierarchy (down = unchecked), void*->T*, enum<->int
const_cast<T>(e)         // add/remove const/volatile ONLY. Writing through a
                         // cast-away-const to an originally-const object is UB
reinterpret_cast<T>(e)   // bit reinterpretation of pointers/refs. Almost always
                         // a strict-aliasing violation if you then dereference
dynamic_cast<T>(e)       // polymorphic downcast, RUNTIME CHECKED (needs RTTI);
                         // returns nullptr for pointers, throws std::bad_cast
                         // for references. Costs a runtime lookup
std::bit_cast<T>(e)      // C++20: reinterpret the OBJECT REPRESENTATION safely.
                         // Requires same size + trivially copyable. constexpr.
(T)e                     // C-style: tries const_cast, static_cast,
                         // reinterpret_cast in order. Never use it in C++
```

**Strict aliasing:** you may only access an object through a pointer/reference to
its own type, a `const`/`volatile` variant, a signed/unsigned variant of it, or
`char`/`unsigned char`/`std::byte`. Violating it is UB and `-O2` will bite you.
`-fno-strict-aliasing` is the escape hatch (the Linux kernel uses it).

```cpp
// WRONG
float f = 1.0f;
std::uint32_t bits = *reinterpret_cast<std::uint32_t*>(&f);   // UB
// RIGHT
auto bits2 = std::bit_cast<std::uint32_t>(f);                 // or memcpy
```

`gsl::narrow_cast` / your own `narrow_cast` is worth writing:
```cpp
template <typename To, typename From>
constexpr To narrow_cast(From v) {
    auto out = static_cast<To>(v);
    assert(static_cast<From>(out) == v && "narrowing lost information");
    return out;
}
```

## 7. `sizeof`, `alignof`, padding

```cpp
struct Bad  { char a; double b; char c; };   // 24 bytes: 1 + 7pad + 8 + 1 + 7pad
struct Good { double b; char a; char c; };   // 16 bytes: 8 + 1 + 1 + 6pad
static_assert(sizeof(Bad)  == 24);
static_assert(sizeof(Good) == 16);
static_assert(alignof(Good) == 8);           // == alignment of the strictest member
```

Rules: `sizeof` a struct is a multiple of its alignment (so arrays stay aligned);
alignment of a struct is the max of its members'; `sizeof(empty class) == 1`
(distinct addresses), but an empty **base** can occupy 0 bytes (EBO), and C++20
`[[no_unique_address]]` extends that to members.

**Order members from largest to smallest.** Use `-Wpadded` to find waste. On an
embedded/FSD target, a 24→16 byte struct is a 33% cut in DRAM traffic for an
array of them — that is a real, defensible optimization.

```cpp
alignas(64) struct CacheLinePadded { std::atomic<int> counter; };
// prevents false sharing; std::hardware_destructive_interference_size (C++17)
```

`sizeof` is an unevaluated context: `sizeof(f())` does not call `f`, and
`sizeof` an array parameter gives you the *pointer* size (arrays decay).

---

## Traps checklist

1. `Widget w();` is a function declaration (most vexing parse).
2. `vector<int> v{3,0}` is 2 elements; `v(3,0)` is 3 elements.
3. Default-init of a built-in at block scope leaves it **indeterminate**; reading
   it is UB. `{}` value-initializes.
4. `delete` vs `delete[]` must match. `delete nullptr` is fine.
5. Forming a pointer more than one past the end is UB, even unread.
6. Reference lifetime extension does **not** apply to function parameters,
   returned references, or reference members in a ctor-init-list.
7. `const int* p` vs `int* const p` — read right to left.
8. `reinterpret_cast` + dereference = strict-aliasing UB; use `std::bit_cast`.
9. `dynamic_cast` costs runtime and needs a polymorphic type.
10. `volatile` is for hardware registers, **not** for threads.
11. Member order changes `sizeof`; order big → small.
12. Never allocate in a real-time hot path.
