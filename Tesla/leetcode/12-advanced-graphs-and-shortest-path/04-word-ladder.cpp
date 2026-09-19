// PROBLEM     Transform begin_word into end_word one letter at a time; every intermediate
//             word must be in the dictionary. Return the number of words in the shortest
//             transformation (0 if impossible).
// APPROACH    This is BFS on an IMPLICIT graph: the nodes are words and the edges are
//             one-letter changes. Do NOT build the adjacency list by comparing every pair
//             (that is O(n^2 * L)). Instead GENERATE the neighbours of a word by trying all
//             26 letters at each of its L positions and testing dictionary membership --
//             O(26 * L) per word.
//             Optimization worth mentioning: BIDIRECTIONAL BFS. Search from both ends and
//             stop when the frontiers meet. If the branching factor is b and the answer is
//             d, that is O(b^(d/2)) instead of O(b^d) -- a genuine square-root speedup,
//             and the version below always expands the SMALLER frontier.
// COMPLEXITY  Time O(N * L * 26) with N dictionary words of length L; space O(N * L).
// FOLLOW-UPS  Return all shortest paths (LC 126)? -> BFS to build a level graph, then
//             backtrack through it -- do not try to collect paths during the BFS.
//             Why generate neighbours instead of building the graph? -> the graph has up
//             to N^2 edges; generation is linear in the dictionary.
//             The "*" wildcard bucket approach preprocesses patterns like "h*t" into
//             buckets -- better when L is large relative to 26.

#include <cassert>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

// Bidirectional BFS: expand whichever frontier is smaller.
int ladder_length(const std::string& begin_word, const std::string& end_word,
                  const std::vector<std::string>& word_list) {
    std::unordered_set<std::string> dict(word_list.begin(), word_list.end());
    if (!dict.contains(end_word)) return 0;
    if (begin_word == end_word) return 1;

    std::unordered_set<std::string> front{begin_word}, back{end_word};
    int length = 1;

    while (!front.empty() && !back.empty()) {
        if (front.size() > back.size()) std::swap(front, back);   // expand the smaller side

        std::unordered_set<std::string> next;
        for (const std::string& word : front) {
            std::string candidate = word;
            for (std::size_t i = 0; i < candidate.size(); ++i) {
                const char original = candidate[i];
                for (char c = 'a'; c <= 'z'; ++c) {
                    if (c == original) continue;
                    candidate[i] = c;
                    if (back.contains(candidate)) return length + 1;   // frontiers met
                    if (dict.erase(candidate) > 0) next.insert(candidate);  // visit once
                }
                candidate[i] = original;
            }
        }
        front = std::move(next);
        ++length;
    }
    return 0;
}

int main() {
    assert(ladder_length("hit", "cog", {"hot", "dot", "dog", "lot", "log", "cog"}) == 5);
    assert(ladder_length("hit", "cog", {"hot", "dot", "dog", "lot", "log"}) == 0);  // no cog
    assert(ladder_length("a", "c", {"a", "b", "c"}) == 2);
    assert(ladder_length("hot", "dog", {"hot", "dog"}) == 0);        // no bridge word
    assert(ladder_length("hot", "dot", {"dot"}) == 2);
    assert(ladder_length("abc", "abc", {"abc"}) == 1);               // already there
    assert(ladder_length("hit", "cog", {}) == 0);
    assert(ladder_length("talk", "tail", {"talk", "tons", "fall", "tail", "gale", "hall",
                                          "negs"}) == 0);
    return 0;
}
