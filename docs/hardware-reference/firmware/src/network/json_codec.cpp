#include "network/json_codec.hpp"

#include <ArduinoJson.h>
#include <cmath>

namespace okanegachi::network {
namespace {

bool is_uuid(const std::string& s) {
  return s.size() == 36 && s[8] == '-' && s[13] == '-' && s[18] == '-' && s[23] == '-';
}

bool reject_fractional(JsonVariantConst v) {
  if (v.is<double>()) {
    const double d = v.as<double>();
    return std::floor(d) != d;
  }
  return false;
}

bool validate_finance(JsonObjectConst obj) {
  if (obj["spend_minor"].is<double>() && reject_fractional(obj["spend_minor"])) return false;
  if (obj["budget_minor"].is<double>() && reject_fractional(obj["budget_minor"])) return false;
  return true;
}

} // namespace

std::string build_sync_request(const SyncRequest& req) {
  JsonDocument doc;
  doc["api_version"] = 1;
  doc["request_id"] = req.request_id;
  doc["device_id"] = req.device_id;
  if (req.epoch_null) doc["epoch"] = nullptr;
  else doc["epoch"] = req.epoch;
  doc["ack_command_seq"] = req.ack_command_seq;
  JsonArray events = doc["events"].to<JsonArray>();
  for (const auto& e : req.events) {
    JsonObject ev = events.add<JsonObject>();
    ev["event_id"] = e.event_id;
    ev["type"] = e.type;
    JsonDocument payload;
    deserializeJson(payload, e.payload_json);
    ev["payload"] = payload.as<JsonVariantConst>();
  }
  JsonObject tel = doc["telemetry"].to<JsonObject>();
  tel["firmware"] = "0.1.0";
  tel["asset_version"] = 1;
  tel["uptime_s"] = req.uptime_s;
  tel["rssi_dbm"] = -55;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::optional<ParsedResponse> parse_sync_response(const std::string& body, const std::string& expected_request_id,
                                                  const std::string& expected_epoch, int cached_state_version) {
  JsonDocument doc;
  const auto err = deserializeJson(doc, body);
  if (err) return std::nullopt;
  if (!doc["api_version"].is<int>() || doc["api_version"].as<int>() != 1) return std::nullopt;
  if (doc.containsKey("error")) return std::nullopt;
  ParsedResponse out;
  out.request_id = doc["request_id"].as<std::string>();
  if (out.request_id != expected_request_id) {
    out.ok = false;
    out.reject_reason = "request_id";
    return out;
  }
  out.epoch = doc["epoch"].as<std::string>();
  if (!expected_epoch.empty() && out.epoch != expected_epoch) {
    out.ok = false;
    out.reject_reason = "epoch";
    return out;
  }
  if (!doc["state_version"].is<int>()) return std::nullopt;
  out.state_version = doc["state_version"].as<int>();
  if (out.state_version < cached_state_version) {
    out.ok = false;
    out.reject_reason = "stale_state";
    return out;
  }
  if (doc["finance"].is<JsonObjectConst>() && !validate_finance(doc["finance"])) {
    out.ok = false;
    out.reject_reason = "finance";
    return out;
  }
  out.server_time = doc["server_time"].as<std::string>();
  out.next_sync_ms = doc["next_sync_ms"].as<int>();
  out.demo_mode = doc["demo_mode"].as<bool>();
  if (doc["care"]["stage"].is<const char*>()) out.care_stage = doc["care"]["stage"].as<std::string>();
  if (doc["commands"].is<JsonArrayConst>()) {
    for (JsonObjectConst cmd : doc["commands"].as<JsonArrayConst>()) {
      Command c;
      c.seq = cmd["seq"].as<int>();
      c.command_id = cmd["command_id"].as<std::string>();
      c.animation = cmd["animation"].as<std::string>();
      c.duration_ms = cmd["duration_ms"].as<int>();
      if (cmd["expires_at"].is<const char*>()) c.expires_at = cmd["expires_at"].as<std::string>();
      out.commands.push_back(c);
    }
  }
  if (doc["event_results"].is<JsonArrayConst>()) {
    for (JsonObjectConst er : doc["event_results"].as<JsonArrayConst>()) {
      EventResult r;
      r.event_id = er["event_id"].as<std::string>();
      r.status = er["status"].as<std::string>();
      out.event_results.push_back(r);
    }
  }
  out.ok = true;
  return out;
}

} // namespace okanegachi::network
