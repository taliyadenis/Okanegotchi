#include "app/fake_platform.hpp"

#include <ArduinoJson.h>
#include <ctime>
#include <fstream>
#include <sstream>

namespace okanegachi::app {

void FakeClock::advance(std::uint32_t ms) {
  monotonic_ms += ms;
  utc_ms += static_cast<std::int64_t>(ms);
}

std::uint32_t FakeClock::millis32() const { return static_cast<std::uint32_t>(monotonic_ms); }

bool FakeStore::put(const std::string& key, const std::vector<std::uint8_t>& value) {
  if (failures_remaining) {
    --failures_remaining;
    return false;
  }
  records_[key] = value;
  return true;
}

bool FakeStore::get(const std::string& key, std::vector<std::uint8_t>& value) const {
  auto it = records_.find(key);
  if (it == records_.end()) return false;
  value = it->second;
  return true;
}

void FakeServer::load_fixture(const std::string& path) {
  std::ifstream in(path);
  std::ostringstream ss;
  ss << in.rdbuf();
  base_json_ = ss.str();
  JsonDocument doc;
  deserializeJson(doc, base_json_);
  if (doc["epoch"].is<const char*>()) epoch_ = doc["epoch"].as<std::string>();
}

std::string FakeServer::merge_patch(const std::string& patch_json, std::int64_t utc_ms) {
  JsonDocument base;
  deserializeJson(base, base_json_);
  JsonDocument patch;
  if (!patch_json.empty()) deserializeJson(patch, patch_json);
  if (!patch_json.empty()) {
    for (JsonPair kv : patch.as<JsonObject>()) {
      base[kv.key()] = kv.value();
    }
  }
  if (!patch["epoch"].is<const char*>()) base["epoch"] = epoch_;
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(utc_ms));
  (void)buf;
  if (!base["server_time"].is<const char*>()) {
    const time_t t = static_cast<time_t>(utc_ms / 1000);
    tm tm{};
    gmtime_r(&t, &tm);
    char iso[32];
    std::snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    base["server_time"] = iso;
  }
  for (JsonObject cmd : base["commands"].as<JsonArray>()) {
    if (!cmd["expires_at"].is<const char*>()) {
      const std::time_t t = static_cast<std::time_t>(utc_ms / 1000) + 120;
      std::tm tm{};
      gmtime_r(&t, &tm);
      char iso[32];
      std::snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
      cmd["expires_at"] = iso;
    }
  }
  std::string out;
  serializeJson(base, out);
  return out;
}

} // namespace okanegachi::app
