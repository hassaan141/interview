#include "shared.hpp"
#include <cstdio>

void report_a();
void report_b();

int main() {
    Widget w;
    std::printf("  main : sizeof(Frame)=%zu  Widget::value()=%d\n",
                sizeof(Frame), w.value());
    report_a();     // bumps g_inline_counter and sets its own s_per_tu_copy
    report_b();     // sees g_inline_counter == 1 but s_per_tu_copy == 0
    return 0;
}
