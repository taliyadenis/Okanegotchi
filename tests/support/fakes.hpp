#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Test helpers only; Cursor adapts these to production dependency interfaces.
namespace okanegachi::test {
struct FakeClock {
    std::uint64_t monotonic_ms{0};
    std::int64_t utc_ms{1790361005000LL}; // 2026-09-25T18:30:05Z
    void advance(std::uint64_t ms) { monotonic_ms += ms; utc_ms += static_cast<std::int64_t>(ms); }
    std::uint32_t millis32() const { return static_cast<std::uint32_t>(monotonic_ms); }
};
struct FakeStore {
    std::map<std::string, std::vector<std::uint8_t>> records;
    unsigned failures_remaining{0};
    unsigned attempts{0};
    unsigned successes{0};
    bool put(const std::string& key, const std::vector<std::uint8_t>& value) {
        ++attempts;
        if (failures_remaining) { --failures_remaining; return false; }
        records[key] = value;
        ++successes;
        return true;
    }
    bool get(const std::string& key, std::vector<std::uint8_t>& value) const {
        auto it = records.find(key);
        if (it == records.end()) return false;
        value = it->second;
        return true;
    }
    // For crash tests, explicitly corrupt committed bytes; never treat as atomic NVS proof.
    void corrupt(const std::string& key, const std::vector<std::uint8_t>& value) { records[key] = value; }
};
struct FakeTransport {
    std::vector<std::string> request_bodies;
    bool in_flight{false};
    bool send(const std::string& body) {
        if (in_flight) return false;
        in_flight = true;
        request_bodies.push_back(body);
        return true;
    }
    void complete() { in_flight = false; }
};
} // namespace okanegachi::test
