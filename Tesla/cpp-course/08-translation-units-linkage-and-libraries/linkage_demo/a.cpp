#include "shared.hpp"
#include <cstdio>

int defined_in_a() { return 42; }

// a.cpp's idea of Frame's layout. If b.cpp was compiled with a different -D, the
// two TUs disagree about where `crc` lives and this reads the wrong bytes.
int consume_frame(const Frame& f) { return static_cast<int>(f.id + f.crc); }

Widget::~Widget() = default;            // without this: undefined reference to vtable
int Widget::value() const { return 7; }

// Same name, internal linkage in both TUs -> two distinct functions, no clash.
namespace { int tu_local_helper() { return 1; } }

void report_a() {
    s_per_tu_copy = 100;                // writes A's OWN copy
    ++g_inline_counter;                 // writes THE ONE shared object
    std::printf("  a.cpp: sizeof(Frame)=%zu  s_per_tu_copy=%d  g_inline_counter=%d  "
                "inline_square(5)=%d  tu_local_helper=%d\n",
                sizeof(Frame), s_per_tu_copy, g_inline_counter,
                inline_square(5), tu_local_helper());
}
