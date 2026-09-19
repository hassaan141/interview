// shared.hpp -- a deliberately educational header.
#pragma once
#include <cstdint>
#include <cstdio>

// (A) inline function: DEFINABLE in every TU, linker merges the definitions.
inline int inline_square(int x) { return x * x; }

// (B) C++17 inline variable: ONE object across the whole program, header-only.
inline int g_inline_counter = 0;

// (C) A non-inline function DECLARATION. Exactly one TU must define it, or the
//     link fails with "undefined reference".
int defined_in_a();

// (D) `static` in a header gives EVERY TU ITS OWN COPY. Almost always a bug in a
//     header -- each TU silently sees a different variable.
static int s_per_tu_copy = 0;   // NOLINT: this is the anti-pattern, on purpose

// (E) THE ODR TRAP. sizeof(Frame) depends on a macro, so two TUs compiled with
//     different -D flags disagree about the layout of the same type. That is
//     ill-formed-no-diagnostic-required: the linker picks one definition and you
//     get silent memory corruption.
struct Frame {
    std::uint32_t id;
#ifdef EXTRA_FIELD
    std::uint64_t extra;
#endif
    std::uint32_t crc;
};

// A Frame crossing a TU boundary is what makes the ODR mismatch observable to
// -Wodr under LTO -- and what makes it corrupt memory without LTO.
int consume_frame(const Frame& f);

// (F) A class with a virtual function declared but not defined in the header:
//     if no TU defines it you get "undefined reference to vtable for Widget".
struct Widget {
    virtual ~Widget();          // declared here, defined in a.cpp
    virtual int value() const;
};
