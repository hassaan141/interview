// PROBLEM     Design an LRU cache with O(1) get and put.
// APPROACH    THE canonical answer: a hash map for O(1) lookup, plus a DOUBLY linked list
//             for O(1) reordering. The map stores an ITERATOR (or node pointer) into the
//             list, so "move this entry to the front" needs no search.
//             In C++ the trick is std::list::splice: it relinks nodes in O(1) and, unlike
//             erase+insert, does NOT invalidate iterators -- which is exactly what lets
//             the map keep holding them.
// COMPLEXITY  get O(1), put O(1) average. Space O(capacity).
// FOLLOW-UPS  Why a doubly linked list? -> you must unlink a node given only the node, in
//             O(1); a singly linked list needs its predecessor.
//             Why not a vector? -> moving an element to the front is O(n).
//             Thread safe? -> a mutex around both structures (a shared_mutex does NOT
//             help: get() MUTATES the recency order). For a real concurrent cache, shard
//             by key hash, or use a CLOCK/second-chance approximation which needs only an
//             atomic reference bit -- that is what production caches actually do.
//             LFU (LC 460)? -> frequency buckets, each a list, plus a min-frequency.
//             Real system: this is a page cache, a tile cache, a map-tile LRU in an
//             autonomy stack -- so the follow-up "how would you bound memory instead of
//             entry count?" is worth preparing (track bytes, evict until under budget).

#include <cassert>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

class LRUCache {
    using Entry = std::pair<int, int>;                 // {key, value}
    std::size_t capacity_;
    std::list<Entry> order_;                            // FRONT = most recently used
    std::unordered_map<int, std::list<Entry>::iterator> index_;

public:
    explicit LRUCache(std::size_t capacity) : capacity_{capacity} {
        index_.reserve(capacity * 2);                   // avoid rehashing at steady state
    }

    std::optional<int> get(int key) {
        const auto it = index_.find(key);
        if (it == index_.end()) return std::nullopt;
        // splice: O(1) relink, and it does NOT invalidate `it->second`.
        order_.splice(order_.begin(), order_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        if (capacity_ == 0) return;
        if (const auto it = index_.find(key); it != index_.end()) {
            it->second->second = value;                 // update in place
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        if (index_.size() == capacity_) {               // evict the least recent
            const auto& [old_key, old_value] = order_.back();
            index_.erase(old_key);
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_.emplace(key, order_.begin());
    }

    std::size_t size() const noexcept { return index_.size(); }
    bool contains(int key) const { return index_.contains(key); }   // does NOT touch order
};

int main() {
    LRUCache c{2};
    c.put(1, 1);
    c.put(2, 2);
    assert(c.get(1) == 1);            // 1 becomes most recent
    c.put(3, 3);                       // evicts 2, not 1
    assert(!c.get(2).has_value());
    assert(c.get(1) == 1 && c.get(3) == 3);
    c.put(4, 4);                       // evicts 1 (3 and 1 were used, 1 less recently)
    assert(!c.get(1).has_value());
    assert(c.get(3) == 3 && c.get(4) == 4);
    assert(c.size() == 2);

    // Updating an existing key must not evict and must refresh recency.
    LRUCache d{2};
    d.put(1, 10); d.put(2, 20);
    d.put(1, 100);
    assert(d.get(1) == 100 && d.size() == 2);
    d.put(3, 30);                      // 2 is now the least recent
    assert(!d.get(2).has_value() && d.get(1) == 100);

    // Capacity 1, and capacity 0.
    LRUCache one{1};
    one.put(1, 1); one.put(2, 2);
    assert(!one.get(1).has_value() && one.get(2) == 2);
    LRUCache zero{0};
    zero.put(1, 1);
    assert(zero.size() == 0 && !zero.get(1).has_value());
    return 0;
}
