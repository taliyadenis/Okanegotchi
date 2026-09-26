#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace okanegachi::app {

struct FakeClock {
  std::uint64_t monotonic_ms{0};
  std::int64_t utc_ms{1790361005000LL};
  void advance(std::uint32_t ms);
  std::uint32_t millis32() const;
};

class FakeStore {
public:
  unsigned failures_remaining{0};
  bool put(const std::string& key, const std::vector<std::uint8_t>& value);
  bool get(const std::string& key, std::vector<std::uint8_t>& value) const;

private:
  std::map<std::string, std::vector<std::uint8_t>> records_;
};

class FakeServer {
public:
  void load_fixture(const std::string& path);
  std::string merge_patch(const std::string& patch_json, std::int64_t utc_ms);
  std::string epoch() const { return epoch_; }
  void set_epoch(const std::string& e) { epoch_ = e; }

private:
  std::string base_json_;
  std::string epoch_{"ef286bf8-928d-46f7-b697-bf2379ccac7b"};
};

} // namespace okanegachi::app
