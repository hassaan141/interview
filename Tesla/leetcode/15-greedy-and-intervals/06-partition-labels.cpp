// PROBLEM     Partition a string into as many parts as possible so that each letter
//             appears in at most one part. Return the part sizes.
// EXAMPLE     "ababcbacadefegdehijhklij" -> [9,7,8]
// APPROACH    Precompute the LAST index of every character. Then sweep, extending the
//             current part's end to the maximum last-index of the characters seen so far.
//             When the scan position reaches that end, no character inside can appear
//             later, so the part is closed -- emit it and start the next.
//             This is the interval idea in disguise: each letter defines an interval
//             [first, last], and a part is a maximal union of overlapping intervals. Seeing
//             that connection is the point.
// COMPLEXITY  Time O(n) (one pass to index, one to sweep), Space O(alphabet) = O(1).
// FOLLOW-UPS  Why is greedy correct? -> a part MUST extend at least to the last occurrence
//             of every letter it contains; closing at exactly that point is therefore both
//             necessary and sufficient, so it is the smallest possible part and the count
//             is maximal (a stays-ahead argument).
//             Unicode? -> the 26-entry array assumption breaks; use a hash map.
//             Return the substrings instead of the sizes? -> track the start index too.

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>
#include <vector>

std::vector<int> partition_labels(std::string_view s) {
    std::array<int, 26> last{};                          // last index of each letter
    last.fill(-1);
    for (std::size_t i = 0; i < s.size(); ++i)
        last[static_cast<std::size_t>(s[i] - 'a')] = static_cast<int>(i);

    std::vector<int> sizes;
    int start = 0, end = 0;
    for (int i = 0; i < static_cast<int>(s.size()); ++i) {
        end = std::max(end, last[static_cast<std::size_t>(s[static_cast<std::size_t>(i)] - 'a')]);
        if (i == end) {                                   // nothing inside recurs later
            sizes.push_back(end - start + 1);
            start = i + 1;
        }
    }
    return sizes;
}

int main() {
    assert((partition_labels("ababcbacadefegdehijhklij") == std::vector<int>{9, 7, 8}));
    assert((partition_labels("eccbbbbdec") == std::vector<int>{10}));   // one part
    assert((partition_labels("abc") == std::vector<int>{1, 1, 1}));     // all separate
    assert(partition_labels("").empty());
    assert((partition_labels("a") == std::vector<int>{1}));
    assert((partition_labels("aa") == std::vector<int>{2}));
    assert((partition_labels("abab") == std::vector<int>{4}));
    assert((partition_labels("abacd") == std::vector<int>{3, 1, 1}));
    return 0;
}
