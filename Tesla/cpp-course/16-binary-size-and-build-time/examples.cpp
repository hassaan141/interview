// 16 — Binary size and build time, measured rather than asserted.
//
// This file is a DRIVER: it compiles copies of itself with different flags and
// reports the resulting sizes, then times a few build configurations.
//
//   g++ -std=c++20 -O2 examples.cpp -o ex && ./ex
//
// It shells out to the compiler, so it needs g++ on PATH (it is, you just used it).

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string run_capture(const std::string& cmd) {
    std::string out;
    if (FILE* p = popen(cmd.c_str(), "r")) {
        std::array<char, 512> buf{};
        while (std::fgets(buf.data(), static_cast<int>(buf.size()), p)) out += buf.data();
        pclose(p);
    }
    return out;
}

static double time_command_s(const std::string& cmd) {
    const auto t0 = std::chrono::steady_clock::now();
    const int rc = std::system((cmd + " > /dev/null 2>&1").c_str());
    const auto t1 = std::chrono::steady_clock::now();
    (void)rc;
    return std::chrono::duration<double>(t1 - t0).count();
}

// A test program with the ingredients that actually drive size and build time:
// heavy headers, many template instantiations, exceptions, and iostreams.
static const char* kHeavySource = R"CPP(
#include <iostream>
#include <map>
#include <memory>
#include <regex>          // notoriously large
#include <sstream>
#include <string>
#include <vector>

template <typename T> struct Box {
    T v{};
    std::string describe() const { std::ostringstream os; os << v; return os.str(); }
    T twice() const { return v + v; }
};

// 24 distinct instantiations -> 24 copies of the code.
template <int N> struct Chain {
    static int value() { return N + Chain<N - 1>::value(); }
};
template <> struct Chain<0> { static int value() { return 0; } };

int main() {
    Box<int> a{1}; Box<long> b{2}; Box<double> c{3.5}; Box<float> d{4.5f};
    Box<short> e{5}; Box<unsigned> f{6}; Box<char> g{'x'}; Box<long long> h{8};
    std::cout << a.describe() << b.twice() << c.describe() << d.twice()
              << e.twice() << f.twice() << g.describe() << h.twice() << '\n';
    std::cout << Chain<24>::value() << '\n';
    try {
        std::regex re("[0-9]+");
        std::cout << std::regex_match("123", re) << '\n';
    } catch (const std::exception& ex) { std::cerr << ex.what(); }
    std::map<std::string, std::vector<std::unique_ptr<int>>> m;
    m["k"].push_back(std::make_unique<int>(1));
    return static_cast<int>(m.size());
}
)CPP";

// The same program with the size-expensive ingredients removed.
static const char* kLeanSource = R"CPP(
#include <cstdio>
#include <span>

template <typename T> T twice(T v) { return v + v; }

int main() {
    std::printf("%d %ld %f\n", twice(1), twice(2L), twice(3.5));
    return 0;
}
)CPP";

struct Row { std::string label; std::uintmax_t bytes{}; double build_s{}; };

static void report(const std::vector<Row>& rows) {
    const std::uintmax_t base = rows.empty() ? 1 : rows.front().bytes;
    std::printf("    %-46s %10s %8s %8s\n", "configuration", "bytes", "vs base", "build s");
    for (const Row& r : rows) {
        std::printf("    %-46s %10ju %7.2fx %8.2f\n", r.label.c_str(), r.bytes,
                    static_cast<double>(r.bytes) / static_cast<double>(base), r.build_s);
    }
}

int main() {
    const fs::path dir = fs::temp_directory_path() / "section16_size";
    fs::create_directories(dir);
    const fs::path heavy = dir / "heavy.cpp";
    const fs::path lean  = dir / "lean.cpp";
    { std::ofstream{heavy} << kHeavySource; }
    { std::ofstream{lean}  << kLeanSource; }

    std::puts("section 16: binary size and build time (measured on THIS machine)\n");

    // ------------------------------------------------- 1. flags vs. size
    std::puts("  1. the same source, different flags");
    struct Cfg { const char* label; const char* flags; };
    const Cfg cfgs[]{
        {"-O2 -g   (unstripped: debug info dominates)", "-O2 -g"},
        {"-O2      (no -g)",                            "-O2"},
        {"-O2 -g   then `strip --strip-all`",           "-O2 -g"},          // stripped below
        {"-O3",                                         "-O3"},
        {"-Os      (optimize for size)",                "-Os"},
        {"-Os -ffunction-sections -Wl,--gc-sections",
         "-Os -ffunction-sections -fdata-sections -Wl,--gc-sections"},
        {"-Os + above + -fvisibility=hidden",
         "-Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fvisibility=hidden"},
        {"-Os + above + -flto",
         "-Os -ffunction-sections -fdata-sections -Wl,--gc-sections -fvisibility=hidden -flto"},
    };

    std::vector<Row> rows;
    int idx = 0;
    for (const Cfg& c : cfgs) {
        const fs::path out = dir / ("heavy_" + std::to_string(idx));
        const std::string cmd = "g++ -std=c++20 " + std::string{c.flags} + " " +
                                heavy.string() + " -o " + out.string();
        const double t = time_command_s(cmd);
        if (!fs::exists(out)) { ++idx; continue; }
        if (idx == 2) (void)!std::system(("strip --strip-all " + out.string()).c_str());
        rows.push_back({c.label, fs::file_size(out), t});
        ++idx;
    }
    report(rows);
    std::puts("    ^ debug info is usually the biggest single component. Strip the");
    std::puts("      shipped binary and keep the symbols separately, or you cannot");
    std::puts("      symbolize a core dump:");
    std::puts("        objcopy --only-keep-debug app app.debug");
    std::puts("        objcopy --add-gnu-debuglink=app.debug app && strip app");

    // ------------------------------------------- 2. exceptions and RTTI
    std::puts("\n  2. what -fno-exceptions / -fno-rtti actually remove");
    const fs::path with_eh = dir / "eh_on";
    const fs::path no_rtti  = dir / "no_rtti";
    time_command_s("g++ -std=c++20 -O2 " + heavy.string() + " -o " + with_eh.string());
    time_command_s("g++ -std=c++20 -O2 -fno-rtti " + heavy.string() + " -o " + no_rtti.string());
    if (fs::exists(with_eh)) {
        std::printf("    baseline -O2                     : %ju bytes\n",
                    fs::file_size(with_eh));
        const std::string sections = run_capture("size -A " + with_eh.string()
                                                 + " | grep -E '\\.(eh_frame|gcc_except_table|text|rodata)'");
        std::printf("    unwind/EH sections in that binary:\n%s", sections.c_str());
    }
    if (fs::exists(no_rtti))
        std::printf("    -fno-rtti                        : %ju bytes\n", fs::file_size(no_rtti));
    std::puts("    (this source USES exceptions, so -fno-exceptions will not compile it --");
    std::puts("     that is the point: it is an ABI-wide decision, not a per-file flag.)");

    // --------------------------------------- 3. heavy vs lean source
    std::puts("\n  3. the SOURCE matters more than the flags");
    const fs::path h_o2 = dir / "h_o2";
    const fs::path l_o2 = dir / "l_o2";
    const double th = time_command_s("g++ -std=c++20 -O2 " + heavy.string() + " -o " + h_o2.string());
    const double tl = time_command_s("g++ -std=c++20 -O2 " + lean.string()  + " -o " + l_o2.string());
    if (fs::exists(h_o2) && fs::exists(l_o2)) {
        std::printf("    heavy (iostream+regex+map+24 templates): %8ju bytes, %.2f s\n",
                    fs::file_size(h_o2), th);
        std::printf("    lean  (cstdio+span, 3 instantiations)  : %8ju bytes, %.2f s\n",
                    fs::file_size(l_o2), tl);
        std::printf("    ratio: %.1fx size, %.1fx build time\n",
                    static_cast<double>(fs::file_size(h_o2)) / static_cast<double>(fs::file_size(l_o2)),
                    th / tl);
    }
    std::puts("    ^ <regex> and <iostream> dominate both. No flag recovers this;");
    std::puts("      only not including them does.");

    // --------------------------------------- 4. where the bytes went
    std::puts("\n  4. attributing the size (bloaty is the real tool; nm works everywhere)");
    std::puts("    biggest symbols in the heavy -O2 binary:");
    std::printf("%s", run_capture("nm -C --size-sort -S " + h_o2.string()
                                  + " 2>/dev/null | tail -8 | sed 's/^/      /'").c_str());
    std::puts("    sections:");
    std::printf("%s", run_capture("size -A " + h_o2.string()
        + R"( 2>/dev/null | grep -E '^\.(text|rodata|data|bss|eh_frame|gcc_except_table|init_array|dynstr|rela)' )"
        + " | sed 's/^/      /'").c_str());

    // --------------------------------------- 5. build time knobs
    std::puts("\n  5. build-time knobs on one TU (a real project also has LINK time)");
    struct BCfg { const char* label; const char* flags; };
    const BCfg bcfgs[]{
        {"-O0",                   "-O0"},
        {"-O2",                   "-O2"},
        {"-O2 -g",                "-O2 -g"},
        {"-O2 -gsplit-dwarf",     "-O2 -g -gsplit-dwarf"},
        {"-O3",                   "-O3"},
        {"-O2 -flto",             "-O2 -flto"},
    };
    std::printf("    %-24s %8s\n", "flags", "seconds");
    for (const BCfg& b : bcfgs) {
        const double t = time_command_s("g++ -std=c++20 " + std::string{b.flags} + " "
                                        + heavy.string() + " -o " + (dir / "bt_out").string());
        std::printf("    %-24s %8.2f\n", b.label, t);
    }
    std::puts("    ^ on a real project: measure with `clang++ -ftime-trace` +");
    std::puts("      ClangBuildAnalyzer, split compile from link, then in order:");
    std::puts("      ccache, ninja, mold/lld, cut the include graph, -gsplit-dwarf,");
    std::puts("      extern template, PCH/unity for cold dirs, then C++20 modules.");

    std::puts("\nsection 16: done. Now run `bloaty` on a real binary and diff two builds.");
    fs::remove_all(dir);
}
