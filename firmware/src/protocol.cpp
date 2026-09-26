#include "protocol.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace okanegachi {
namespace {
// Bounds ArduinoJson's heap even for hostile input with many tiny values.
class BudgetAllocator : public ArduinoJson::Allocator {
  size_t used_{};
  struct alignas(std::max_align_t) Header {
    size_t size;
  };

public:
  void *allocate(size_t n) override {
    if (n > 32768 - used_)
      return nullptr;
    auto *h = static_cast<Header *>(std::malloc(sizeof(Header) + n));
    if (!h)
      return nullptr;
    h->size = n;
    used_ += n;
    return h + 1;
  }
  void deallocate(void *p) override {
    if (p) {
      auto *h = static_cast<Header *>(p) - 1;
      used_ -= h->size;
      std::free(h);
    }
  }
  void *reallocate(void *p, size_t n) override {
    if (!p)
      return allocate(n);
    auto *h = static_cast<Header *>(p) - 1;
    const size_t old = h->size;
    if (n > 32768 - (used_ - old))
      return nullptr;
    auto *next = static_cast<Header *>(std::realloc(h, sizeof(Header) + n));
    if (!next)
      return nullptr;
    next->size = n;
    used_ = used_ - old + n;
    return next + 1;
  }
};
// ArduinoJson can stop at the first complete value. Reject trailing junk and
// mismatched framing before decoding. JSON syntax itself is checked by the
// decoder.
bool envelope(const std::string &s) {
  size_t first = s.find_first_not_of(" \r\n\t");
  if (first == s.npos || s[first] != '{')
    return false;
  char stack[12];
  size_t depth = 0;
  bool quoted = false, escape = false;
  for (size_t i = first; i < s.size(); ++i) {
    char c = s[i];
    if (quoted) {
      if (escape)
        escape = false;
      else if (c == '\\')
        escape = true;
      else if (c == '"')
        quoted = false;
      else if (static_cast<unsigned char>(c) < 32)
        return false;
      continue;
    }
    if (c == '"')
      quoted = true;
    else if (c == '{' || c == '[') {
      if (depth == 12)
        return false;
      stack[depth++] = c;
    } else if (c == '}' || c == ']') {
      if (!depth || stack[--depth] != (c == '}' ? '{' : '['))
        return false;
      if (!depth)
        return s.find_first_not_of(" \r\n\t", i + 1) == s.npos;
    }
  }
  return false;
}
int digits(const std::string &s, size_t at, size_t n) {
  if (at + n > s.size())
    return -1;
  int value = 0;
  for (size_t i = at; i < at + n; ++i) {
    if (s[i] < '0' || s[i] > '9')
      return -1;
    value = value * 10 + s[i] - '0';
  }
  return value;
}
int64_t days_from_civil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = unsigned(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  return int64_t(era) * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}
} // namespace

bool valid_uuid(const std::string &s) {
  if (s.size() != 36)
    return false;
  for (size_t i = 0; i < s.size(); ++i) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (s[i] != '-')
        return false;
    } else if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f') ||
                 (s[i] >= 'A' && s[i] <= 'F')))
      return false;
  }
  return true;
}
uint64_t retry_after_ms(const std::string &value, int64_t now) {
  if (value.empty() || value.size() > 64)
    return 0;
  bool numeric = value.size() <= 10;
  uint64_t seconds = 0;
  if (numeric)
    for (char c : value) {
      if (c < '0' || c > '9') {
        numeric = false;
        break;
      }
      seconds = seconds * 10 + unsigned(c - '0');
    }
  if (numeric && seconds <= UINT32_MAX)
    return seconds * 1000;
  if (value.size() != 29 || value.substr(3, 2) != ", " ||
      value.substr(25) != " GMT")
    return 0;
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int month = 0;
  for (int i = 0; i < 12; ++i)
    if (value.substr(8, 3) == months[i])
      month = i + 1;
  if (!month)
    return 0;
  char stamp[32];
  snprintf(stamp, sizeof(stamp), "%s-%02d-%sT%sZ", value.substr(12, 4).c_str(),
           month, value.substr(5, 2).c_str(), value.substr(17, 8).c_str());
  auto expiry = utc_millis(stamp);
  return expiry && *expiry > now ? uint64_t(*expiry - now) : 0;
}
bool valid_firmware_version(const std::string &s) {
  if (s.empty() || s.size() > 32)
    return false;
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z'))
      continue;
    if (i && (c == '.' || c == '_' || c == '+' || c == '-'))
      continue;
    return false;
  }
  return true;
}
int utf8_length(const char *s, size_t bytes) {
  int count = 0;
  for (size_t i = 0; i < bytes;) {
    const auto c = static_cast<unsigned char>(s[i]);
    size_t n;
    uint32_t cp;
    if (c < 128) {
      n = 1;
      cp = c;
    } else if (c >= 0xC2 && c <= 0xDF) {
      n = 2;
      cp = c & 31;
    } else if (c >= 0xE0 && c <= 0xEF) {
      n = 3;
      cp = c & 15;
    } else if (c >= 0xF0 && c <= 0xF4) {
      n = 4;
      cp = c & 7;
    } else
      return -1;
    if (i + n > bytes)
      return -1;
    for (size_t j = 1; j < n; ++j) {
      auto b = static_cast<unsigned char>(s[i + j]);
      if ((b & 0xC0) != 0x80)
        return -1;
      cp = (cp << 6) | (b & 63);
    }
    if ((n == 2 && cp < 128) || (n == 3 && cp < 2048) ||
        (n == 4 && cp < 65536) || cp > 0x10FFFF ||
        (cp >= 0xD800 && cp <= 0xDFFF))
      return -1;
    i += n;
    ++count;
  }
  return count;
}
std::optional<int64_t> utc_millis(const std::string &s, bool date_only) {
  if ((date_only && s.size() != 10) ||
      (!date_only && (s.size() < 20 || s.size() > 24)))
    return {};
  int y = digits(s, 0, 4), m = digits(s, 5, 2), d = digits(s, 8, 2);
  if (y < 1 || m < 1 || m > 12 || d < 1 || s[4] != '-' || s[7] != '-')
    return {};
  const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  const int maxday = month_days[m - 1] +
                     (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)));
  if (d > maxday)
    return {};
  int h = 0, mi = 0, se = 0, ms = 0;
  if (!date_only) {
    h = digits(s, 11, 2);
    mi = digits(s, 14, 2);
    se = digits(s, 17, 2);
    if (s[10] != 'T' || s[13] != ':' || s[16] != ':' || s.back() != 'Z' ||
        h < 0 || h > 23 || mi < 0 || mi > 59 || se < 0 || se > 59)
      return {};
    if (s.size() != 20) {
      if (s.size() < 22 || s[19] != '.')
        return {};
      size_t n = s.size() - 21;
      ms = digits(s, 20, n);
      if (ms < 0)
        return {};
      if (n == 1)
        ms *= 100;
      if (n == 2)
        ms *= 10;
    }
  }
  return ((days_from_civil(y, unsigned(m), unsigned(d)) * 24 + h) * 3600 +
          mi * 60 + se) *
             1000 +
         ms;
}
std::string iso_utc(int64_t ms) {
  int64_t seconds = ms / 1000, days = seconds / 86400, rem = seconds % 86400;
  if (rem < 0) {
    rem += 86400;
    --days;
  }
  int64_t z = days + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = unsigned(z - era * 146097),
                 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = int(yoe) + int(era) * 400;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100),
           mp = (5 * doy + 2) / 153;
  unsigned d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9;
  y += m <= 2;
  char out[32];
  std::snprintf(out, sizeof out, "%04d-%02u-%02uT%02d:%02d:%02dZ", y, m, d,
                int(rem / 3600), int(rem / 60 % 60), int(rem % 60));
  return out;
}
namespace {
#include "generated/schema_checks.inc"
Finance finance(JsonVariantConst v) {
  Finance f;
  f.version = v["snapshot_version"];
  f.spend = v["spend_minor"];
  if (!v["budget_minor"].isNull())
    f.budget = v["budget_minor"].as<int>();
  f.source = v["data_source"].as<std::string>();
  f.as_of = v["as_of"].as<std::string>();
  f.start = v["period_start"].as<std::string>();
  f.end = v["period_end"].as<std::string>();
  f.status = v["budget_status"].as<std::string>();
  f.summary = v["summary"].as<std::string>();
  return f;
}
std::optional<Goal> goal(JsonVariantConst v) {
  if (v.isNull())
    return {};
  return Goal{v["name"].as<std::string>(), v["target_minor"].as<int>(),
              v["saved_minor"].as<int>()};
}
bool parse(const std::string &body, Schema schema, JsonDocument &doc) {
  if (body.empty() ||
      body.size() >
          (schema == Schema::Request ? kRequestLimit : kResponseLimit) ||
      !envelope(body))
    return false;
  if (deserializeJson(doc, body, DeserializationOption::NestingLimit(12)))
    return false;
  return schema_check(doc.as<JsonVariantConst>(), schema);
}
} // namespace
bool validate_json(const std::string &body, Schema schema, std::string *error) {
  BudgetAllocator alloc;
  JsonDocument doc(&alloc);
  bool ok = parse(body, schema, doc);
  if (error)
    *error = ok ? "" : "INVALID_JSON_CONTRACT";
  return ok;
}
std::optional<Response> decode_response(const std::string &body) {
  BudgetAllocator alloc;
  JsonDocument doc(&alloc);
  if (!parse(body, Schema::Response, doc))
    return {};
  Response r;
  r.request_id = doc["request_id"].as<std::string>();
  r.epoch = doc["epoch"].as<std::string>();
  r.server_time = doc["server_time"].as<std::string>();
  r.version = doc["state_version"];
  r.poll_ms = doc["next_sync_ms"];
  r.demo = doc["demo_mode"];
  auto p = doc["pet"];
  r.state.appearance = {
      p["pet_id"].as<std::string>(), p["name"].as<std::string>(),
      p["palette"].as<std::string>(), p["accessory"].as<std::string>()};
  auto c = doc["care"];
  r.state.stage = c["stage"].as<std::string>();
  r.state.timezone = c["timezone"].as<std::string>();
  r.state.local_date = c["local_date"].as<std::string>();
  r.state.window = c["current_window"].as<std::string>();
  r.state.connected_ms = c["elapsed_connected_ms"];
  r.state.streak = c["streak_days"];
  r.state.am = c["am_complete"];
  r.state.pm = c["pm_complete"];
  r.state.finance = finance(doc["finance"]);
  r.state.goal = goal(doc["goal"]);
  if (!doc["review"].isNull()) {
    auto v = doc["review"];
    r.state.review = Review{v["review_id"].as<std::string>(),
                            v["expires_at"].as<std::string>(),
                            v["financial_snapshot_version"].as<int>(),
                            finance(v["finance"]), goal(v["goal"])};
    if (r.state.review->version != r.state.review->finance.version)
      return {};
  }
  for (auto v : doc["commands"].as<JsonArrayConst>())
    r.commands.push_back({v["seq"].as<int>(), v["command_id"].as<std::string>(),
                          v["animation"].as<std::string>(),
                          v["expires_at"].as<std::string>(),
                          v["duration_ms"].as<int>()});
  for (auto v : doc["event_results"].as<JsonArrayConst>())
    r.results.push_back(
        {v["event_id"].as<std::string>(), v["status"].as<std::string>(),
         v["reason"].isNull() ? "" : v["reason"].as<std::string>()});
  return r;
}
std::string encode_request(const std::string &id, const std::string &device,
                           const std::string &epoch, int ack,
                           const std::vector<Event> &events, uint64_t uptime,
                           int rssi) {
  BudgetAllocator alloc;
  JsonDocument doc(&alloc);
  doc["api_version"] = 1;
  doc["request_id"] = id;
  doc["device_id"] = device;
  if (epoch.empty())
    doc["epoch"] = nullptr;
  else
    doc["epoch"] = epoch;
  doc["ack_command_seq"] = epoch.empty() ? 0 : ack;
  auto a = doc["events"].to<JsonArray>();
  if (!epoch.empty())
    for (const auto &e : events) {
      auto v = a.add<JsonObject>();
      v["event_id"] = e.id;
      v["type"] = e.type;
      JsonDocument p;
      if (deserializeJson(p, e.payload))
        return {};
      v["payload"] = p.as<JsonVariantConst>();
    }
  auto t = doc["telemetry"].to<JsonObject>();
  t["firmware"] = "0.2.0";
  t["asset_version"] = 1;
  t["uptime_s"] = std::min<uint64_t>(uptime, UINT32_MAX);
  t["rssi_dbm"] = std::clamp(rssi, -127, 0);
  std::string result;
  serializeJson(doc, result);
  if (!validate_json(result, Schema::Request))
    return {};
  return result;
}
std::string money(int64_t cents) {
  char b[40];
  std::snprintf(b, sizeof b, "$%lld.%02lld",
                static_cast<long long>(cents / 100),
                static_cast<long long>(std::abs(cents % 100)));
  return b;
}
std::string display_text(const std::string &s, size_t limit) {
  std::string out;
  for (size_t i = 0; i < s.size() && out.size() < limit;) {
    auto c = static_cast<unsigned char>(s[i]);
    if (c >= 32 && c < 127) {
      out += char(c);
      ++i;
    } else {
      out += '?';
      ++i;
      while (i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80)
        ++i;
    }
  }
  return out;
}
} // namespace okanegachi
