// bugs.cpp — one function per defect class. Each is triggered by an argv selector so
// you can watch a specific sanitizer catch a specific bug.
//
//   bash run.sh        # runs every case under the right sanitizer
//
// DO NOT "fix" these. Breaking them is the point.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

// (1) ASan: heap-buffer-overflow
static void heap_overflow() {
    std::vector<int> v(4);
    int* p = v.data();
    p[4] = 1;                          // one past the end
    std::printf("heap_overflow: wrote %d\n", p[4]);
}

// (2) ASan: use-after-free
static void use_after_free() {
    auto* v = new std::vector<int>{1, 2, 3};
    const int* p = v->data();
    delete v;
    std::printf("use_after_free: read %d\n", *p);
}

// (3) ASan: use-after-return (a dangling pointer to a local)
static const int* dangling() {
    const int local = 42;
    return &local;                     // -Wreturn-local-addr also catches this
}
static void use_after_return() {
    std::printf("use_after_return: read %d\n", *dangling());
}

// (4) ASan: stack-buffer-overflow via an off-by-one loop
static void stack_overflow_write() {
    char buf[8]{};
    for (int i = 0; i <= 8; ++i) buf[i] = 'x';    // <= is the bug
    std::printf("stack_overflow_write: %.8s\n", buf);
}

// (5) LSan: a leak
static void leak() {
    auto* p = new std::string(128, 'x');
    std::printf("leak: allocated %zu bytes\n", p->size());
    // never deleted
}

// (6) UBSan: signed integer overflow
static void signed_overflow() {
    int x = std::numeric_limits<int>::max();
    volatile int y = x + 1;            // UB; the optimizer is allowed to assume it
    std::printf("signed_overflow: %d\n", y);
}

// (7) UBSan: shift past the width
static void bad_shift(int n) {
    volatile int x = 1;
    std::printf("bad_shift: %d\n", x << n);       // n == 32 is UB
}

// (8) UBSan: misaligned access through a reinterpret_cast
static void misaligned() {
    alignas(8) char buf[16]{};
    auto* p = reinterpret_cast<std::uint64_t*>(buf + 1);   // 1-byte offset
    *p = 0xdeadbeef;                                       // misaligned store
    std::printf("misaligned: %llx\n", static_cast<unsigned long long>(*p));
}

// (9) TSan: a data race on a plain int
static void data_race() {
    int counter = 0;                       // not atomic, no mutex
    std::thread a{[&] { for (int i = 0; i < 100000; ++i) ++counter; }};
    std::thread b{[&] { for (int i = 0; i < 100000; ++i) ++counter; }};
    a.join(); b.join();
    std::printf("data_race: counter = %d (expected 200000)\n", counter);
}

// (10) TSan: lock-order inversion -> a potential deadlock, reported even if it
// does not actually deadlock on this run.
static void lock_inversion() {
    std::mutex m1, m2;
    std::thread a{[&] { std::scoped_lock l1{m1}; std::this_thread::yield();
                        std::lock_guard l2{m2}; }};
    std::thread b{[&] { std::lock_guard l2{m2}; std::this_thread::yield();
                        std::lock_guard l1{m1}; }};
    a.join(); b.join();
    std::printf("lock_inversion: survived this time\n");
}

// (11) An uninitialized read (MSan; also -ftrivial-auto-var-init=pattern makes it
// deterministic, and -Wmaybe-uninitialized sometimes catches it)
static void uninitialized() {
    struct Config { int timeout_ms; bool verbose; };
    Config c;                                    // default-init: INDETERMINATE
    std::printf("uninitialized: timeout=%d verbose=%d\n", c.timeout_ms, c.verbose);
}

// (12) An assertion / invariant violation, for the EXPECT_DEATH style of test
static void check_failure() {
    const std::size_t index = 10, size = 4;
    if (index >= size) {
        std::fprintf(stderr, "CHECK failed: index < size (%zu < %zu)\n", index, size);
        std::abort();
    }
}

int main(int argc, char** argv) {
    const std::string which = argc > 1 ? argv[1] : "list";
    if      (which == "heap_overflow")       heap_overflow();
    else if (which == "use_after_free")      use_after_free();
    else if (which == "use_after_return")    use_after_return();
    else if (which == "stack_overflow")      stack_overflow_write();
    else if (which == "leak")                leak();
    else if (which == "signed_overflow")     signed_overflow();
    else if (which == "bad_shift")           bad_shift(32);
    else if (which == "misaligned")          misaligned();
    else if (which == "data_race")           data_race();
    else if (which == "lock_inversion")      lock_inversion();
    else if (which == "uninitialized")       uninitialized();
    else if (which == "check_failure")       check_failure();
    else {
        std::puts("cases: heap_overflow use_after_free use_after_return stack_overflow\n"
                  "       leak signed_overflow bad_shift misaligned\n"
                  "       data_race lock_inversion uninitialized check_failure");
    }
    return 0;
}
