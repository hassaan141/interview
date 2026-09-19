// PROBLEM     CPU tasks labelled A-Z, and a cooldown n: two runs of the SAME task must be
//             at least n+1 slots apart. Minimum number of slots (including idles) to run
//             everything?
// EXAMPLE     tasks = [A,A,A,B,B,B], n = 2 -> 8  (A B _ A B _ A B)
// APPROACH    Two answers worth knowing:
//             (a) GREEDY + HEAP, which simulates the schedule: always run the available
//                 task with the highest remaining count, and park it in a cooldown queue
//                 until slot t + n + 1. This generalizes to real scheduling problems.
//             (b) The O(1) counting formula: the most frequent task defines the skeleton.
//                 With max_count occurrences and `ties` tasks at that count:
//                     slots = max((max_count - 1) * (n + 1) + ties, total_tasks)
//                 The max() handles the case where there are so many distinct tasks that
//                 no idling is needed at all.
// COMPLEXITY  (a) O(total * log 26) = O(total). (b) O(total) to count, O(1) after.
// FOLLOW-UPS  Output the actual schedule, not just the length? -> the simulation gives it
//             for free; the formula does not. That is the reason to know both.
//             This IS a real problem: cooldown = a rate limit or a resource that needs to
//             recover, and the greedy "run the most-remaining task" is exactly what a
//             least-laxity / most-remaining-work scheduler does. Worth saying for an
//             autonomy systems role.

#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <vector>

// (a) Simulation with a max-heap plus a cooldown queue.
int least_interval_simulation(const std::vector<char>& tasks, int n) {
    std::array<int, 26> count{};
    for (char c : tasks) ++count[static_cast<std::size_t>(c - 'A')];

    std::priority_queue<int> ready;                      // remaining counts, max first
    for (int c : count) if (c > 0) ready.push(c);

    std::queue<std::pair<int, int>> cooling;             // {remaining, available_at}
    int time = 0;
    while (!ready.empty() || !cooling.empty()) {
        ++time;
        if (!ready.empty()) {
            const int remaining = ready.top() - 1;
            ready.pop();
            if (remaining > 0) cooling.emplace(remaining, time + n);   // park it
        }
        // Anything whose cooldown expired becomes runnable again.
        if (!cooling.empty() && cooling.front().second == time) {
            ready.push(cooling.front().first);
            cooling.pop();
        }
    }
    return time;
}

// (b) The closed form.
int least_interval_formula(const std::vector<char>& tasks, int n) {
    if (tasks.empty()) return 0;              // else max_count is 0 and `ties` counts all
    std::array<int, 26> count{};              // 26 zero entries -- a real edge-case bug
    for (char c : tasks) ++count[static_cast<std::size_t>(c - 'A')];
    const int max_count = *std::ranges::max_element(count);
    const int ties = static_cast<int>(std::ranges::count(count, max_count));
    const int skeleton = (max_count - 1) * (n + 1) + ties;
    return std::max(skeleton, static_cast<int>(tasks.size()));   // max(): no idling needed
}

int main() {
    const auto check = [](auto&& fn) {
        assert(fn(std::vector<char>{'A', 'A', 'A', 'B', 'B', 'B'}, 2) == 8);
        assert(fn(std::vector<char>{'A', 'A', 'A', 'B', 'B', 'B'}, 0) == 6);  // no cooldown
        assert(fn(std::vector<char>{'A', 'A', 'A', 'A', 'A', 'A',
                                    'B', 'C', 'D', 'E', 'F', 'G'}, 2) == 16);
        assert(fn(std::vector<char>{'A'}, 5) == 1);
        assert(fn(std::vector<char>{}, 3) == 0);
        assert(fn(std::vector<char>{'A', 'B', 'C', 'D'}, 1) == 4);           // never idles
        assert(fn(std::vector<char>{'A', 'A'}, 3) == 5);                      // A _ _ _ A
    };
    check([](const std::vector<char>& t, int n) { return least_interval_simulation(t, n); });
    check([](const std::vector<char>& t, int n) { return least_interval_formula(t, n); });
    return 0;
}
