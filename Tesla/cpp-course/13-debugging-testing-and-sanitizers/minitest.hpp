// minitest.hpp — a ~70-line test framework, so this section runs with no
// dependencies. It mirrors the GoogleTest API you will actually use at work:
//   TEST(Suite, Name) { EXPECT_EQ(...); ASSERT_TRUE(...); EXPECT_NEAR(...); }
#pragma once
#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <string>
#include <vector>

namespace minitest {

struct Case { std::string suite, name; std::function<void()> fn; };

inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& failures() { static int f = 0; return f; }
inline bool& current_failed() { static bool b = false; return b; }

struct Registrar {
    Registrar(std::string suite, std::string name, std::function<void()> fn) {
        registry().push_back({std::move(suite), std::move(name), std::move(fn)});
    }
};

inline void fail(const char* file, int line, const std::string& msg) {
    std::printf("    FAIL %s:%d  %s\n", file, line, msg.c_str());
    ++failures();
    current_failed() = true;
}

struct FatalAssertion : std::exception {};

inline int run_all() {
    int passed = 0;
    std::string last_suite;
    for (const auto& c : registry()) {
        if (c.suite != last_suite) { std::printf("  [%s]\n", c.suite.c_str()); last_suite = c.suite; }
        current_failed() = false;
        try { c.fn(); }
        catch (const FatalAssertion&) { /* already reported */ }
        catch (const std::exception& e) {
            fail(__FILE__, 0, std::string{"unexpected exception: "} + e.what());
        }
        if (!current_failed()) { ++passed; std::printf("    ok   %s\n", c.name.c_str()); }
        else                     std::printf("    ---- %s\n", c.name.c_str());
    }
    std::printf("  %d/%zu passed, %d assertion failure(s)\n",
                passed, registry().size(), failures());
    return failures() == 0 ? 0 : 1;
}

}  // namespace minitest

#define MT_CONCAT_(a, b) a##b
#define MT_UNIQUE_(a, b) MT_CONCAT_(a, b)

#define TEST(suite, name)                                                        \
    static void MT_UNIQUE_(mt_##suite##_##name, _fn)();                          \
    static const ::minitest::Registrar MT_UNIQUE_(mt_reg_##suite##_##name, _r){  \
        #suite, #name, MT_UNIQUE_(mt_##suite##_##name, _fn)};                    \
    static void MT_UNIQUE_(mt_##suite##_##name, _fn)()

#define EXPECT_TRUE(cond)                                                        \
    do { if (!(cond)) ::minitest::fail(__FILE__, __LINE__, "expected true: " #cond); } while (0)
#define EXPECT_FALSE(cond)                                                       \
    do { if (cond) ::minitest::fail(__FILE__, __LINE__, "expected false: " #cond); } while (0)
#define EXPECT_EQ(a, b)                                                          \
    do { if (!((a) == (b))) ::minitest::fail(__FILE__, __LINE__, #a " == " #b); } while (0)
#define EXPECT_NE(a, b)                                                          \
    do { if ((a) == (b)) ::minitest::fail(__FILE__, __LINE__, #a " != " #b); } while (0)
// Floating point NEVER uses EXPECT_EQ. This is the whole reason EXPECT_NEAR exists.
#define EXPECT_NEAR(a, b, tol)                                                   \
    do { if (!(std::fabs((a) - (b)) <= (tol)))                                    \
             ::minitest::fail(__FILE__, __LINE__, #a " ~= " #b); } while (0)
#define EXPECT_THROW(stmt, Ex)                                                   \
    do { bool threw = false;                                                      \
         try { stmt; } catch (const Ex&) { threw = true; } catch (...) {}         \
         if (!threw) ::minitest::fail(__FILE__, __LINE__, "expected " #Ex); } while (0)
#define ASSERT_TRUE(cond)                                                        \
    do { if (!(cond)) { ::minitest::fail(__FILE__, __LINE__, "fatal: " #cond);    \
                        throw ::minitest::FatalAssertion{}; } } while (0)
