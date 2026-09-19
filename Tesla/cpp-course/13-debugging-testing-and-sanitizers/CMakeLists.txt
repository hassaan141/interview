# A target-based CMake setup you can defend in an interview.
# Build:  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
#         cmake --build build -j && ctest --test-dir build --output-on-failure
# Sanitizers: cmake -S . -B build-asan -DENABLE_SANITIZERS=ON && ...
cmake_minimum_required(VERSION 3.20)
project(section13_testing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)               # -std=c++20, never -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)       # feeds clang-tidy, clangd, IWYU

# ---- options ---------------------------------------------------------------
option(ENABLE_SANITIZERS "Build with AddressSanitizer + UBSan" OFF)
option(ENABLE_TSAN       "Build with ThreadSanitizer (cannot combine with ASan)" OFF)
option(ENABLE_WERROR     "Treat warnings as errors (CI should set this ON)" OFF)
option(ENABLE_COVERAGE   "Instrument for gcov/lcov coverage" OFF)

# ---- a shared set of warnings, as an INTERFACE target ----------------------
# This is the idiom: an interface library carrying flags, linked by every target.
# It keeps flags identical across TUs, which is also the ODR defence (section 08).
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion
    -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast
    -Wnull-dereference -Wdouble-promotion -Wformat=2
    -Wimplicit-fallthrough -Wmisleading-indentation
    $<$<BOOL:${ENABLE_WERROR}>:-Werror>)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)
if(ENABLE_SANITIZERS)
    target_compile_options(project_options INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer -g)
    target_link_options(project_options INTERFACE -fsanitize=address,undefined)
endif()
if(ENABLE_TSAN)
    target_compile_options(project_options INTERFACE -fsanitize=thread -g)
    target_link_options(project_options INTERFACE -fsanitize=thread)
endif()
if(ENABLE_COVERAGE)
    target_compile_options(project_options INTERFACE --coverage -O0 -g)
    target_link_options(project_options INTERFACE --coverage)
endif()

# ---- the tests -------------------------------------------------------------
include(CTest)
enable_testing()

add_executable(section13_tests examples.cpp)
target_link_libraries(section13_tests PRIVATE project_options project_warnings)
add_test(NAME section13_tests COMMAND section13_tests)

# In a real project this is where GoogleTest comes in. Uncomment to fetch it:
#
# include(FetchContent)
# FetchContent_Declare(googletest
#     GIT_REPOSITORY https://github.com/google/googletest.git
#     GIT_TAG        v1.15.2)
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)
# target_link_libraries(section13_tests PRIVATE GTest::gtest_main GTest::gmock)
# include(GoogleTest)
# gtest_discover_tests(section13_tests)      # one CTest entry per TEST(), so a
#                                            # failure names the case, and -j works

# ---- convenience targets ---------------------------------------------------
add_custom_target(sanitizer_lab
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/sanitizer_lab/run.sh
    COMMENT "Run the sanitizer demonstrations")
