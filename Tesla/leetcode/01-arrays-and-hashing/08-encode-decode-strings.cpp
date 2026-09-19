// PROBLEM     Encode a list of strings into one string, and decode it back. The strings
//             may contain ANY characters, including your delimiter.
// EXAMPLE     ["lint","co,de","love","you"] -> "4#lint5#co,de4#love3#you" -> back again
// APPROACH    LENGTH PREFIXING. A delimiter alone is unsound because the payload can
//             contain it; escaping works but is fiddly and O(n) in the worst case per
//             character. Prefix each string with its length and a separator that cannot
//             appear in a decimal length ('#'): "<len>#<payload>". The decoder reads
//             digits until '#', then takes exactly that many bytes.
// COMPLEXITY  Encode O(total), Decode O(total), Space O(total). Single pass each way.
// FOLLOW-UPS  Why not a delimiter + escaping? -> length prefixing needs no escaping and
//             the decoder never has to look ahead. This is exactly how real wire formats
//             work (protobuf varint length-delimited fields, HTTP Content-Length,
//             netstrings) -- say that, it is the actual point of the question for a
//             foundations role.
//             Hostile input? -> the decoder MUST validate that the claimed length fits in
//             the remaining buffer, or you read out of bounds. See the bounds check.
//             Binary data? -> already works; the payload is opaque bytes.
//             Fixed-width length instead? -> 4 bytes little-endian: no parsing, but you
//             must pin the byte order (section 02 of the course).

#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

std::string encode(const std::vector<std::string>& parts) {
    std::size_t total = 0;
    for (const auto& p : parts) total += p.size() + 12;     // rough, avoids reallocation
    std::string out;
    out.reserve(total);
    for (const std::string& p : parts) {
        out += std::to_string(p.size());
        out += '#';
        out += p;
    }
    return out;
}

// Returns nullopt on malformed input instead of crashing or asserting -- this is a
// PARSER, and a parser must never trust its input (course section 13).
std::optional<std::vector<std::string>> decode(std::string_view s) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < s.size()) {
        const std::size_t hash = s.find('#', i);
        if (hash == std::string_view::npos) return std::nullopt;     // no separator

        std::size_t len = 0;
        if (hash == i) return std::nullopt;                          // empty length field
        for (std::size_t d = i; d < hash; ++d) {
            if (s[d] < '0' || s[d] > '9') return std::nullopt;        // non-digit
            constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();
            if (len > (kMax - 9) / 10) return std::nullopt;           // overflow guard
            len = len * 10 + static_cast<std::size_t>(s[d] - '0');
        }
        const std::size_t start = hash + 1;
        if (len > s.size() - start) return std::nullopt;   // THE bounds check: a hostile
                                                           // length must not read past
        out.emplace_back(s.substr(start, len));
        i = start + len;
    }
    return out;
}

int main() {
    const std::vector<std::string> cases[]{
        {"lint", "code", "love", "you"},
        {"co,de", "with#hash", "4#tricky"},     // payloads containing the delimiters
        {"", "", ""},                            // empty strings
        {},                                      // empty list
        {std::string(1000, 'x')},                // long payload
        {std::string("\0binary\0", 8)},          // embedded NULs
    };
    for (const auto& parts : cases) {
        const auto round_tripped = decode(encode(parts));
        assert(round_tripped.has_value());
        assert(*round_tripped == parts);
    }

    // Malformed input must be rejected, not crash. (Run this file under ASan.)
    assert(!decode("4#abc").has_value());        // claims 4 bytes, has 3
    assert(!decode("abc").has_value());          // no '#'
    assert(!decode("#abc").has_value());         // empty length
    assert(!decode("x#abc").has_value());        // non-digit length
    assert(!decode("99999999999999999999#a").has_value());   // overflowing length
    assert(decode("").has_value() && decode("")->empty());
    return 0;
}
