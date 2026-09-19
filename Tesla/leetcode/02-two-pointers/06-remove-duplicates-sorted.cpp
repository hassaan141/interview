// PROBLEM     Remove duplicates in place from a SORTED array; return the new length. The
//             first k elements must hold the unique values in order.
// EXAMPLE     [1,1,2] -> 2, array becomes [1,2,...]
// APPROACH    Read/write two pointers. `write` lags `read`; copy forward only when the
//             value differs from the last kept one. This is exactly what std::unique
//             does, and saying so is the right answer in C++.
// COMPLEXITY  Time O(n), Space O(1). One pass, one comparison and at most one move per
//             element.
// FOLLOW-UPS  Allow each value at most TWICE? -> compare against v[write-2] instead
//             (LeetCode 80) -- the same skeleton with a different predicate.
//             Unsorted? -> you need a hash set, O(n) space, and the order question
//             becomes real. Non-trivial element type? -> std::move, not copy.

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

// The hand-written version, because the interviewer wants to see the pointers.
std::size_t remove_duplicates(std::vector<int>& v) {
    if (v.empty()) return 0;
    std::size_t write = 1;
    for (std::size_t read = 1; read < v.size(); ++read)
        if (v[read] != v[write - 1]) v[write++] = v[read];
    return write;
}

// Allow at most `k` copies of each value -- the same skeleton, one changed predicate.
std::size_t remove_duplicates_at_most(std::vector<int>& v, std::size_t k) {
    std::size_t write = 0;
    for (std::size_t read = 0; read < v.size(); ++read)
        if (write < k || v[read] != v[write - k]) v[write++] = v[read];
    return write;
}

// The C++ answer: say this exists, then write the manual one if asked.
std::size_t remove_duplicates_stl(std::vector<int>& v) {
    const auto [first, last] = std::ranges::unique(v);       // moves, does not erase
    const auto n = static_cast<std::size_t>(first - v.begin());
    v.erase(first, last);                                     // the part people forget
    return n;
}

int main() {
    std::vector<int> a{1, 1, 2};
    assert(remove_duplicates(a) == 2 && a[0] == 1 && a[1] == 2);

    std::vector<int> b{0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    assert(remove_duplicates(b) == 5);
    assert((std::vector<int>(b.begin(), b.begin() + 5) == std::vector<int>{0, 1, 2, 3, 4}));

    std::vector<int> c{};
    assert(remove_duplicates(c) == 0);
    std::vector<int> d{7};
    assert(remove_duplicates(d) == 1);
    std::vector<int> e{5, 5, 5, 5};
    assert(remove_duplicates(e) == 1);

    std::vector<int> f{1, 1, 1, 2, 2, 3};
    assert(remove_duplicates_at_most(f, 2) == 5);            // [1,1,2,2,3]
    assert((std::vector<int>(f.begin(), f.begin() + 5) == std::vector<int>{1, 1, 2, 2, 3}));

    std::vector<int> g{1, 1, 2, 3, 3};
    assert(remove_duplicates_stl(g) == 3 && g == std::vector<int>({1, 2, 3}));
    return 0;
}
