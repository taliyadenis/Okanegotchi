#include "demo.hpp"
#include <algorithm>
namespace okanegachi {
namespace {
const char *base =
#include "generated/demo_base.inc"
    ;
} // namespace
DemoServer::DemoServer(Clock &c, Random &r) : clock_(c), random_(r) { reset(); }
void DemoServer::reset() {
  deserializeJson(state_, base);
  epoch_ = random_.uuid();
  seq_ = 0;
  commands_.clear();
  receipts_.clear();
  state_["epoch"] = epoch_;
  state_["state_version"] = 1;
  state_["pet"]["pet_id"] = "gator";
  state_["pet"]["palette"] = "original";
  state_["pet"]["accessory"] = "none";
  state_["care"]["stage"] = "content";
  state_["care"]["elapsed_connected_ms"] = 0;
  state_["care"]["am_complete"] = false;
  state_["care"]["pm_complete"] = false;
  state_["finance"]["spend_minor"] = 2500;
  state_["finance"]["snapshot_version"] = 1;
  state_["goal"]["saved_minor"] = 5000;
  state_["review"] = nullptr;
  state_["commands"].to<JsonArray>();
  state_["event_results"].to<JsonArray>();
  refresh_finance();
}
void DemoServer::refresh_finance() {
  auto f = state_["finance"];
  int spend = f["spend_minor"] | 0;
  f["as_of"] = iso_utc(clock_.utc());
  f["summary"] = money(spend) + " of $100.00 weekly budget used.";
  f["budget_status"] = spend > 10000 ? "over_budget" : "on_track";
}
void DemoServer::command(const char *animation, int duration) {
  commands_.push_back({++seq_, random_.uuid(), animation,
                       iso_utc(clock_.utc() + 120000), duration});
}
bool DemoServer::scenario(const std::string &scenario) {
  if (scenario == "reset") {
    reset();
    return true;
  }
  if (commands_.size() >= 32)
    return false;
  if (scenario == "food") {
    state_["finance"]["spend_minor"] = std::min(
        2147483647LL, state_["finance"]["spend_minor"].as<int64_t>() + 1250);
    command("eating");
  } else if (scenario == "ride") {
    state_["finance"]["spend_minor"] = std::min(
        2147483647LL, state_["finance"]["spend_minor"].as<int64_t>() + 1800);
    command("traveling");
  } else if (scenario == "savings") {
    state_["goal"]["saved_minor"] = std::min(
        2147483647LL, state_["goal"]["saved_minor"].as<int64_t>() + 1000);
    command("celebrate");
  } else if (scenario == "neglect") {
    state_["care"]["stage"] = "ghost";
    state_["care"]["streak_days"] = 0;
    state_["care"]["elapsed_connected_ms"] = 172800000;
  } else
    return false;
  state_["state_version"] = state_["state_version"].as<int>() + 1;
  state_["finance"]["snapshot_version"] =
      state_["finance"]["snapshot_version"].as<int>() + 1;
  refresh_finance();
  return true;
}
bool DemoServer::choose_pet(const std::string &pet) {
  if (pet != "gator" && pet != "robot" && pet != "duck")
    return false;
  state_["pet"]["pet_id"] = pet;
  state_["state_version"] = state_["state_version"].as<int>() + 1;
  return true;
}
std::pair<int, std::string> DemoServer::sync(const Request &req) {
  if (!validate_json(req.body, Schema::Request))
    return {400, ""};
  JsonDocument input;
  deserializeJson(input, req.body);
  if (!input["epoch"].isNull() && input["epoch"] != epoch_) {
    JsonDocument e;
    e["api_version"] = 1;
    e["request_id"] = req.id;
    e["error"]["code"] = "RESET_REQUIRED";
    e["error"]["message"] = "Local ledger restarted";
    e["error"]["current_epoch"] = epoch_;
    e["error"]["retry_after_ms"] = nullptr;
    std::string out;
    serializeJson(e, out);
    return {409, out};
  }
  while (!commands_.empty() &&
         commands_.front().seq <= input["ack_command_seq"].as<int>())
    commands_.pop_front();
  auto results = state_["event_results"].to<JsonArray>();
  for (auto event : input["events"].as<JsonArrayConst>()) {
    auto id = event["event_id"].as<std::string>();
    auto type = event["type"].as<std::string>();
    std::string signature;
    serializeJson(event, signature);
    auto previous =
        std::find_if(receipts_.begin(), receipts_.end(),
                     [&](const auto &item) { return item.second.id == id; });
    EventResult result{id, "accepted", ""};
    if (previous != receipts_.end()) {
      result = previous->second;
      if (previous->first != signature) {
        result.status = "rejected";
        result.reason = "EVENT_ID_REUSE";
      } else if (result.status == "accepted")
        result.status = "duplicate";
    } else {
      if (type == "demo_trigger") {
        if (!scenario(event["payload"]["scenario"].as<std::string>())) {
          result.status = "rejected";
          result.reason = "INVALID_ACTION";
        }
      } else if (type == "review_requested") {
        auto review = state_["review"].to<JsonObject>();
        review["review_id"] = random_.uuid();
        review["expires_at"] = iso_utc(clock_.utc() + 300000);
        review["financial_snapshot_version"] =
            state_["finance"]["snapshot_version"];
        review["finance"] = state_["finance"];
        review["goal"] = state_["goal"];
        state_["state_version"] = state_["state_version"].as<int>() + 1;
      } else if (type == "checkin_completed") {
        auto review = state_["review"];
        auto expires = utc_millis(review["expires_at"] | "");
        if (review.isNull() ||
            event["payload"]["review_id"] != review["review_id"]) {
          result.status = "rejected";
          result.reason = "REVIEW_NOT_FOUND";
        } else if (!expires || clock_.utc() >= *expires) {
          result.status = "rejected";
          result.reason = "REVIEW_EXPIRED";
        } else {
          bool ghost = state_["care"]["stage"] == "ghost";
          state_["care"]["stage"] = "content";
          state_["care"]["elapsed_connected_ms"] = 0;
          state_["care"]["pm_complete"] = true;
          state_["review"] = nullptr;
          state_["state_version"] = state_["state_version"].as<int>() + 1;
          if (ghost)
            command("revive", 2000);
        }
      }
      receipts_.push_back({signature, result});
      if (receipts_.size() > 64)
        receipts_.pop_front();
    }
    auto v = results.add<JsonObject>();
    v["event_id"] = result.id;
    v["status"] = result.status;
    if (result.reason.empty())
      v["reason"] = nullptr;
    else
      v["reason"] = result.reason;
  }
  state_["request_id"] = req.id;
  state_["server_time"] = iso_utc(clock_.utc());
  auto list = state_["commands"].to<JsonArray>();
  if (!input["epoch"].isNull())
    for (const auto &c : commands_) {
      if (list.size() == 4)
        break;
      auto v = list.add<JsonObject>();
      v["seq"] = c.seq;
      v["command_id"] = c.id;
      v["animation"] = c.animation;
      v["duration_ms"] = c.duration;
      v["expires_at"] = c.expires;
    }
  std::string out;
  serializeJson(state_, out);
  return {200, out};
}
} // namespace okanegachi
