#pragma once
#include <ArduinoJson.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace okanegachi {
constexpr size_t kRequestLimit = 4096, kResponseLimit = 8192;
struct Event {
  std::string id, type, payload;
};
struct Command {
  int seq{};
  std::string id, animation, expires;
  int duration{};
  bool operator==(const Command &b) const {
    return seq == b.seq && id == b.id && animation == b.animation &&
           expires == b.expires && duration == b.duration;
  }
};
struct Finance {
  int version{}, spend{};
  std::optional<int> budget;
  std::string source, as_of, start, end, status, summary;
};
struct Goal {
  std::string name;
  int target{}, saved{};
};
struct Review {
  std::string id, expires;
  int version{};
  Finance finance;
  std::optional<Goal> goal;
};
struct Appearance {
  std::string pet{"gator"}, name{"Okanegachi"}, palette{"original"},
      accessory{"none"};
};
struct State {
  Appearance appearance;
  std::string stage{"content"}, timezone, local_date, window;
  uint64_t connected_ms{};
  int streak{};
  bool am{}, pm{};
  Finance finance;
  std::optional<Goal> goal;
  std::optional<Review> review;
};
struct EventResult {
  std::string id, status, reason;
};
struct Response {
  std::string request_id, epoch, server_time;
  int version{}, poll_ms{};
  bool demo{};
  State state;
  std::vector<Command> commands;
  std::vector<EventResult> results;
};
enum class Schema { Request, Response, Error };
// Calendar and Unicode validation are shared by the generated schema
// validators.
bool valid_uuid(const std::string &value);
int utf8_length(const char *value, size_t bytes);
std::optional<int64_t> utc_millis(const std::string &text,
                                  bool date_only = false);
std::string iso_utc(int64_t millis);
uint64_t retry_after_ms(const std::string &header, int64_t utc_now);
bool valid_firmware_version(const std::string &value);
bool validate_json(const std::string &body, Schema schema,
                   std::string *error = nullptr);
std::optional<Response> decode_response(const std::string &body);
std::string encode_request(const std::string &request_id,
                           const std::string &device_id,
                           const std::string &epoch, int ack,
                           const std::vector<Event> &events, uint64_t uptime,
                           int rssi);
std::string money(int64_t cents);
std::string display_text(const std::string &utf8, size_t limit = 120);
} // namespace okanegachi
