// COMPONENT   (a) A versioned key-value store: set(key, value, timestamp), and
//             get(key, timestamp) returning the value in effect AT that time.
//             (b) A TTL cache with lazy expiry.
// WHERE       Configuration snapshots ("what was the calibration at t=...?"), log replay
//             lookups, and any cache whose entries must not outlive their validity.
//             Time-travel lookup is exactly what an evaluation pipeline needs when
//             replaying a logged drive (course section 13).
//
// DESIGN DECISIONS:
//   - Each key owns a vector of (timestamp, value) sorted by timestamp. Appends are O(1)
//     when timestamps are non-decreasing (which is the normal case for a log), and the
//     lookup is a BINARY SEARCH: the newest entry with timestamp <= the query, i.e.
//     upper_bound minus one. That "upper_bound then step back" is the idiom to remember.
//   - A vector, not a std::map per key: contiguous storage, far better cache behaviour,
//     and a binary search is the same O(log n) with a much smaller constant (course
//     section 12).
//   - Out-of-order writes are handled by inserting at the right position rather than
//     silently corrupting the ordering -- and the cost of that is documented.
//   - The TTL cache expires entries LAZILY on access plus a bounded sweep, because a
//     timer thread would add a wakeup and a lock to every entry.
//
// COMPLEXITY  set: O(1) amortized for in-order timestamps, O(n) for an out-of-order
//             insert. get: O(log n). Space O(total versions).
//
// FOLLOW-UPS  Memory growth? -> versions accumulate forever. Bound it: keep the last N,
//             or compact anything older than the oldest possible query. SAY THIS -- an
//             unbounded store is the real bug in this design.
//             Thread safe? -> a shared_mutex helps here because get() does NOT mutate
//             (unlike an LRU), so many readers genuinely proceed in parallel.
//             Range queries ("all values between t1 and t2")? -> two binary searches.
//             Persistence? -> this is an append-only log, which is exactly how an LSM
//             tree or an MVCC database stores versions.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class TimeMap {
    struct Version { std::int64_t timestamp; std::string value; };
    std::unordered_map<std::string, std::vector<Version>> data_;

public:
    void set(const std::string& key, std::string value, std::int64_t timestamp) {
        auto& versions = data_[key];
        if (versions.empty() || versions.back().timestamp <= timestamp) {
            versions.push_back({timestamp, std::move(value)});     // the common path: O(1)
            return;
        }
        // Out of order: keep the vector sorted. O(n), and worth calling out.
        const auto pos = std::ranges::upper_bound(versions, timestamp, {}, &Version::timestamp);
        versions.insert(pos, {timestamp, std::move(value)});
    }

    // The value in effect at `timestamp`: the newest version with version.ts <= timestamp.
    std::optional<std::string> get(const std::string& key, std::int64_t timestamp) const {
        const auto it = data_.find(key);
        if (it == data_.end() || it->second.empty()) return std::nullopt;
        const auto& versions = it->second;
        // upper_bound gives the first version STRICTLY AFTER the query; step back one.
        const auto after = std::ranges::upper_bound(versions, timestamp, {}, &Version::timestamp);
        if (after == versions.begin()) return std::nullopt;        // nothing that old
        return std::prev(after)->value;
    }

    // Bounding the growth: drop everything strictly older than `keep_from`, except the
    // one version still in effect at that instant.
    void compact(std::int64_t keep_from) {
        for (auto& [key, versions] : data_) {
            const auto after = std::ranges::upper_bound(versions, keep_from, {},
                                                        &Version::timestamp);
            if (after == versions.begin()) continue;
            versions.erase(versions.begin(), std::prev(after));     // keep the effective one
        }
    }
    std::size_t version_count(const std::string& key) const {
        const auto it = data_.find(key);
        return it == data_.end() ? 0 : it->second.size();
    }
};

// ------------------------------------------------------------------- TTL cache
template <typename Clock>
class TtlCache {
    struct Entry { std::string value; std::int64_t expires_at; };
    std::unordered_map<std::string, Entry> map_;
    std::int64_t ttl_ns_;

public:
    explicit TtlCache(std::int64_t ttl_ms) : ttl_ns_{ttl_ms * 1'000'000} {}

    void put(const std::string& key, std::string value) {
        map_[key] = Entry{std::move(value), Clock::now() + ttl_ns_};
    }
    std::optional<std::string> get(const std::string& key) {
        const auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        if (it->second.expires_at <= Clock::now()) {     // LAZY expiry on access
            map_.erase(it);
            return std::nullopt;
        }
        return it->second.value;
    }
    // A bounded sweep, so expired-but-never-accessed entries cannot leak forever.
    std::size_t sweep() {
        const std::int64_t now = Clock::now();
        return std::erase_if(map_, [now](const auto& kv) { return kv.second.expires_at <= now; });
    }
    std::size_t size() const noexcept { return map_.size(); }
};

struct FakeClock {
    static std::int64_t now_ns;
    static std::int64_t now() noexcept { return now_ns; }
    static void advance_ms(std::int64_t ms) { now_ns += ms * 1'000'000; }
};
std::int64_t FakeClock::now_ns = 0;

int main() {
    // ---- versioned store -------------------------------------------------------
    TimeMap store;
    store.set("speed", "10", 1);
    assert(store.get("speed", 1) == "10");
    assert(store.get("speed", 3) == "10");          // still in effect later
    assert(!store.get("speed", 0).has_value());     // before any version existed
    assert(!store.get("missing", 5).has_value());

    store.set("speed", "20", 4);
    assert(store.get("speed", 3) == "10");          // the OLD value, at the old time
    assert(store.get("speed", 4) == "20");          // exactly at the boundary
    assert(store.get("speed", 100) == "20");

    // Two versions with the same timestamp: the later set wins for that instant.
    store.set("mode", "a", 10);
    store.set("mode", "b", 10);
    assert(store.get("mode", 10) == "b");

    // Out-of-order writes must not corrupt the ordering.
    TimeMap unordered;
    unordered.set("k", "late", 100);
    unordered.set("k", "early", 10);
    unordered.set("k", "middle", 50);
    assert(unordered.get("k", 10) == "early");
    assert(unordered.get("k", 50) == "middle");
    assert(unordered.get("k", 99) == "middle");
    assert(unordered.get("k", 100) == "late");
    assert(!unordered.get("k", 9).has_value());

    // Compaction bounds the memory while preserving the effective value.
    assert(unordered.version_count("k") == 3);
    unordered.compact(60);
    assert(unordered.version_count("k") == 2);       // "middle" (still effective) + "late"
    assert(unordered.get("k", 60) == "middle");
    assert(unordered.get("k", 100) == "late");

    // ---- TTL cache -------------------------------------------------------------
    FakeClock::now_ns = 0;
    TtlCache<FakeClock> cache{/*ttl_ms=*/100};
    cache.put("a", "1");
    assert(cache.get("a") == "1");

    FakeClock::advance_ms(99);
    assert(cache.get("a") == "1");                   // not yet expired
    FakeClock::advance_ms(1);
    assert(!cache.get("a").has_value());             // exactly at the TTL: expired
    assert(cache.size() == 0);                        // and evicted on access

    // Entries that are never read still get cleaned up by the sweep.
    cache.put("x", "1");
    cache.put("y", "2");
    assert(cache.size() == 2);
    FakeClock::advance_ms(200);
    assert(cache.size() == 2);                        // lazy: still resident
    assert(cache.sweep() == 2);                       // the sweep reclaims them
    assert(cache.size() == 0);

    // Re-putting a key refreshes its expiry.
    cache.put("z", "old");
    FakeClock::advance_ms(50);
    cache.put("z", "new");
    FakeClock::advance_ms(60);
    assert(cache.get("z") == "new");                  // would have expired without the put
    return 0;
}
