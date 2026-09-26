#pragma once
#include <optional>
#include <string>
#include <vector>

namespace okanegachi::network {

struct OutboxEvent {
  std::string event_id;
  std::string type;
  std::string payload_json;
};

struct SyncRequest {
  std::string request_id;
  std::string device_id;
  std::string epoch;
  bool epoch_null{false};
  int ack_command_seq{0};
  std::vector<OutboxEvent> events;
  int uptime_s{0};
};

struct Command {
  int seq{0};
  std::string command_id;
  std::string animation;
  int duration_ms{0};
  std::string expires_at;
};

struct EventResult {
  std::string event_id;
  std::string status;
};

struct ParsedResponse {
  bool ok{false};
  std::string reject_reason;
  std::string request_id;
  std::string epoch;
  std::string server_time;
  int state_version{0};
  int next_sync_ms{2000};
  bool demo_mode{false};
  std::string care_stage;
  std::vector<Command> commands;
  std::vector<EventResult> event_results;
};

std::string build_sync_request(const SyncRequest& req);
std::optional<ParsedResponse> parse_sync_response(const std::string& body, const std::string& expected_request_id,
                                                  const std::string& expected_epoch, int cached_state_version);

} // namespace okanegachi::network
