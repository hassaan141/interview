#include "shared.hpp"
#include <cstdio>

// Same name as in a.cpp but with internal linkage -> no "multiple definition".
namespace { int tu_local_helper() { return 2; } }

void report_b() {
    Frame f{};
    f.id  = 1;
    f.crc = 2;
    // Built with b.cpp's layout, consumed with a.cpp's layout.
    const int consumed = consume_frame(f);
    std::printf("  b.cpp: consume_frame({id=1,crc=2}) = %d (expected 3)\n", consumed);
    std::printf("  b.cpp: sizeof(Frame)=%zu  s_per_tu_copy=%d  g_inline_counter=%d  "
                "defined_in_a()=%d  tu_local_helper=%d\n",
                sizeof(Frame), s_per_tu_copy, g_inline_counter,
                defined_in_a(), tu_local_helper());
}
