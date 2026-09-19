// PROBLEM     For each day, how many days until a warmer temperature? 0 if never.
// EXAMPLE     [73,74,75,71,69,72,76,73] -> [1,1,4,2,1,1,0,0]
// APPROACH    Monotonic stack of INDICES with decreasing temperatures. When today is
//             warmer than the top of the stack, today RESOLVES that pending day: the
//             answer is the index difference. Repeat until the stack top is warmer.
// COMPLEXITY  Time O(n) amortized -- each index is pushed once and popped once, so the
//             inner while is O(1) on average. Space O(n). Brute force is O(n^2).
// FOLLOW-UPS  This is literally "next greater element". LC 496/503 (circular) are the
//             same loop; for circular, iterate 2n times modulo n.
//             Next SMALLER? -> flip the comparison. To the LEFT? -> iterate backwards.
//             Stock span (LC 901)? -> the mirror image, previous greater element.

#include <cassert>
#include <vector>

std::vector<int> daily_temperatures(const std::vector<int>& temps) {
    std::vector<int> answer(temps.size(), 0);         // 0 = "no warmer day"
    std::vector<std::size_t> stack;                    // indices, temps DECREASING
    stack.reserve(temps.size());

    for (std::size_t i = 0; i < temps.size(); ++i) {
        while (!stack.empty() && temps[stack.back()] < temps[i]) {
            answer[stack.back()] = static_cast<int>(i - stack.back());
            stack.pop_back();
        }
        stack.push_back(i);
    }
    return answer;                                     // anything left keeps its 0
}

int main() {
    assert((daily_temperatures({73, 74, 75, 71, 69, 72, 76, 73})
            == std::vector<int>{1, 1, 4, 2, 1, 1, 0, 0}));
    assert((daily_temperatures({30, 40, 50, 60}) == std::vector<int>{1, 1, 1, 0}));
    assert((daily_temperatures({30, 60, 90}) == std::vector<int>{1, 1, 0}));
    assert((daily_temperatures({90, 80, 70}) == std::vector<int>{0, 0, 0}));  // decreasing
    assert((daily_temperatures({50, 50, 50}) == std::vector<int>{0, 0, 0}));  // all equal
    assert((daily_temperatures({42}) == std::vector<int>{0}));
    assert(daily_temperatures({}).empty());
    return 0;
}
