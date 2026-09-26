#include "app/app_engine.hpp"

#include "nfc/ndef_parser.hpp"

#include <ArduinoJson.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace okanegachi::app {
namespace {

std::string utc_iso(std::int64_t utc_ms) {
  const std::time_t t = static_cast<std::time_t>(utc_ms / 1000);
  std::tm tm{};
  gmtime_r(&t, &tm);
  char iso[32];
  std::snprintf(iso, sizeof(iso), "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
  return iso;
}

bool parse_expired(const std::string& expires_at, std::int64_t utc_ms) {
  if (expires_at.empty()) return false;
  std::tm tm{};
  if (std::sscanf(expires_at.c_str(), "%d-%d-%dT%d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour,
                  &tm.tm_min, &tm.tm_sec) < 6)
    return false;
  tm.tm_year -= 1900;
  tm.tm_mon -= 1;
  const std::time_t t = timegm(&tm);
  return static_cast<std::int64_t>(t) * 1000 < utc_ms;
}

std::string new_uuid() {
  static std::uint32_t ctr = 1;
  char buf[64];
  std::snprintf(buf, sizeof(buf), "aaaaaaaa-bbbb-4ccc-8ddd-%012u", ctr++);
  return buf;
}

} // namespace

void AppEngine::emit_trace(const std::string& type, const std::map<std::string, std::string>& extra) {
  TraceEntry e;
  e.fields["type"] = type;
  for (const auto& kv : extra) e.fields[kv.first] = kv.second;
  trace_.push_back(e);
}

std::string AppEngine::page_name() const {
  switch (page_) {
  case Page::Pet: return "Pet";
  case Page::Goal: return "Goal";
  case Page::CheckIn: return "Check-in";
  case Page::Menu: return "Menu";
  case Page::Review: return "Review";
  case Page::Game: return "Game";
  }
  return "Pet";
}

std::string AppEngine::net_name() const {
  switch (net_) {
  case NetStatus::Ready: return "ready";
  case NetStatus::Backoff: return "backoff";
  case NetStatus::Unauthorized: return "unauthorized";
  case NetStatus::Forbidden: return "forbidden";
  case NetStatus::UpdateRequired: return "update_required";
  case NetStatus::Error: return "error";
  }
  return "ready";
}

bool AppEngine::demo_fresh() const {
  if (!demo_mode_) return false;
  if (last_demo_sync_ms_ == 0) return false;
  return clock_.utc_ms - last_demo_sync_ms_ <= 10000;
}

void AppEngine::persist_state() {
  JsonDocument doc;
  doc["ack"] = ack_seq_;
  doc["epoch"] = epoch_;
  doc["state_version"] = state_version_;
  doc["care"] = care_stage_;
  doc["reset_pending"] = reset_pending_;
  JsonArray ob = doc["outbox"].to<JsonArray>();
  for (const auto& e : outbox_) {
    JsonObject o = ob.add<JsonObject>();
    o["event_id"] = e.event_id;
    o["type"] = e.type;
    o["payload"] = e.payload_json;
  }
  std::string s;
  serializeJson(doc, s);
  std::vector<std::uint8_t> bytes(s.begin(), s.end());
  if (!store_.put("state", bytes)) {
    storage_fault_ = true;
    emit_trace("storage_error", {});
  }
}

void AppEngine::load_state() {
  std::vector<std::uint8_t> bytes;
  if (!store_.get("state", bytes)) return;
  const std::string s(bytes.begin(), bytes.end());
  JsonDocument doc;
  deserializeJson(doc, s);
  if (doc["ack"].is<int>()) ack_seq_ = doc["ack"].as<int>();
  if (doc["epoch"].is<const char*>()) epoch_ = doc["epoch"].as<std::string>();
  if (doc["state_version"].is<int>()) state_version_ = doc["state_version"].as<int>();
  if (doc["care"].is<const char*>()) care_stage_ = doc["care"].as<std::string>();
  if (doc["reset_pending"].is<bool>()) reset_pending_ = doc["reset_pending"].as<bool>();
  outbox_.clear();
  for (JsonObject o : doc["outbox"].as<JsonArray>()) {
    network::OutboxEvent e;
    e.event_id = o["event_id"].as<std::string>();
    e.type = o["type"].as<std::string>();
    e.payload_json = o["payload"].as<std::string>();
    outbox_.push_back(e);
  }
}

bool AppEngine::save_cursor(int seq) {
  const int previous = ack_seq_;
  ack_seq_ = seq;
  persist_state();
  if (storage_fault_) {
    ack_seq_ = previous;
    return false;
  }
  TraceEntry e;
  e.fields["type"] = "cursor_saved";
  e.fields["epoch"] = epoch_;
  e.seq = seq;
  trace_.push_back(e);
  return true;
}

bool AppEngine::enqueue_event(const network::OutboxEvent& ev) {
  if (storage_fault_) return false;
  if (outbox_.size() >= 8) return false;
  outbox_.push_back(ev);
  persist_state();
  if (storage_fault_) {
    outbox_.pop_back();
    return false;
  }
  emit_trace("event_queued", {{"event_type", ev.type}, {"event_id", ev.event_id}});
  sync_due_ = true;
  return true;
}

void AppEngine::handle_buttons() {
  // handled via callbacks registered in init
}

void AppEngine::init_from_seed(const std::string& seed_json, const std::string& fixture_path) {
  server_.load_fixture(fixture_path);
  trace_.clear();
  requests_.clear();
  command_queue_.clear();
  active_command_.reset();
  active_animation_ = "idle";
  page_ = Page::Pet;
  storage_fault_ = false;
  request_in_flight_ = false;
  unauthorized_ = false;
  reset_pending_ = false;
  pending_demo_intent_.reset();
  outbox_.clear();
  epoch_ = server_.epoch();
  ack_seq_ = 4;
  state_version_ = 18;
  demo_mode_ = true;
  next_sync_ms_ = 2000;
  care_stage_ = "content";
  paired_ = true;
  online_ = true;
  next_sync_at_ms_ = UINT32_MAX;
  sync_due_ = true;
  last_demo_sync_ms_ = 0;

  JsonDocument seed;
  deserializeJson(seed, seed_json);
  if (seed["paired"].is<bool>() && !seed["paired"].as<bool>()) {
    paired_ = false;
    epoch_.clear();
    ack_seq_ = 0;
  }
  if (seed["ack"].is<int>()) ack_seq_ = seed["ack"].as<int>();
  if (seed["ghost"].is<bool>() && seed["ghost"].as<bool>()) {
    ghost_seed_ = true;
    care_stage_ = "ghost";
  }
  if (seed["monotonic_ms"].is<std::uint64_t>()) clock_.monotonic_ms = seed["monotonic_ms"].as<std::uint64_t>();
  buttons_.sync_time(clock_.monotonic_ms);
  if (seed["buttons_held"].is<JsonArrayConst>()) {
    for (const char* b : seed["buttons_held"].as<JsonArrayConst>()) {
      if (std::strcmp(b, "A") == 0) buttons_.set_held_on_boot(core::ButtonId::A, true);
      if (std::strcmp(b, "B") == 0) buttons_.set_held_on_boot(core::ButtonId::B, true);
      if (std::strcmp(b, "C") == 0) buttons_.set_held_on_boot(core::ButtonId::C, true);
    }
  }
  if (seed["outbox"].is<JsonArrayConst>()) {
    for (JsonObjectConst o : seed["outbox"].as<JsonArrayConst>()) {
      network::OutboxEvent e;
      e.event_id = o["event_id"].as<std::string>();
      e.type = o["type"].as<std::string>();
      serializeJson(o["payload"], e.payload_json);
      outbox_.push_back(e);
    }
  }

  buttons_.set_trace([this](const core::ButtonTrace& t) {
    emit_trace(t.type, {{"button", t.button}});
    if (t.type == "button_short") {
      if (t.button == "C") {
        if (page_ == Page::Pet) page_ = Page::Goal;
        else if (page_ == Page::Goal) page_ = Page::CheckIn;
        else if (page_ == Page::CheckIn) page_ = Page::Menu;
        else page_ = Page::Pet;
      }
    } else if (t.type == "button_long") {
      if (t.button == "B" && page_ == Page::Pet) {
        network::OutboxEvent e;
        e.event_id = new_uuid();
        e.type = "review_requested";
        e.payload_json = "{\"source\":\"button\"}";
        enqueue_event(e);
      } else if (t.button == "C" && page_ == Page::Pet && care_stage_ != "ghost") {
        page_ = Page::Game;
      }
    }
  });
}

void AppEngine::reboot() {
  const auto saved_trace = trace_;
  const auto saved_requests = requests_;
  load_state();
  command_queue_.clear();
  active_command_.reset();
  active_animation_ = "idle";
  request_in_flight_ = false;
  unauthorized_ = false;
  pending_demo_intent_.reset();
  sync_due_ = true;
  last_request_ms_ = 0;
  trace_ = saved_trace;
  requests_ = saved_requests;
  emit_trace("reboot", {});
}

void AppEngine::try_send_sync() {
  if (unauthorized_ || storage_fault_) return;
  if (request_in_flight_) return;
  if (!sync_due_ && clock_.millis32() < next_sync_at_ms_) return;
  if (last_request_ms_ && clock_.monotonic_ms - last_request_ms_ < 1000) return;

  network::SyncRequest req;
  req.request_id = new_uuid();
  req.device_id = device_id_;
  req.epoch_null = reset_pending_ || !paired_ || epoch_.empty();
  req.epoch = epoch_;
  req.ack_command_seq = ack_seq_;
  req.events = outbox_;
  req.uptime_s = static_cast<int>(clock_.monotonic_ms / 1000);
  const std::string body = network::build_sync_request(req);
  requests_.push_back(body);
  pending_request_id_ = req.request_id;
  submitted_event_ids_.clear();
  for (const auto& e : outbox_) submitted_event_ids_.push_back(e.event_id);
  request_in_flight_ = true;
  sync_due_ = false;
  last_request_ms_ = static_cast<std::uint32_t>(clock_.monotonic_ms);
}

void AppEngine::apply_event_results(const std::vector<network::EventResult>& results) {
  for (const auto& r : results) {
    const bool submitted =
        std::find(submitted_event_ids_.begin(), submitted_event_ids_.end(), r.event_id) != submitted_event_ids_.end();
    if (!submitted) {
      emit_trace("response_rejected", {{"reason", "unsolicited_result"}});
      continue;
    }
    outbox_.erase(std::remove_if(outbox_.begin(), outbox_.end(),
                                 [&](const network::OutboxEvent& e) { return e.event_id == r.event_id; }),
                  outbox_.end());
    persist_state();
  }
}

void AppEngine::apply_commands(const std::vector<network::Command>& commands, bool ghost) {
  std::vector<network::Command> sorted = commands;
  std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.seq < b.seq; });
  for (const auto& c : sorted) {
    if (c.seq <= ack_seq_) continue;
    if (parse_expired(c.expires_at, clock_.utc_ms)) {
      save_cursor(c.seq);
      continue;
    }
    const bool ordinary = c.animation != "revive";
    if (ghost && ordinary) {
      save_cursor(c.seq);
      continue;
    }
    if (!save_cursor(c.seq)) return;
    PendingCommand pc{c.seq, c.animation, c.duration_ms, c.expires_at};
    if (!active_command_.has_value()) {
      active_command_ = pc;
      active_animation_ = pc.animation;
      active_anim_end_ms_ = clock_.millis32() + static_cast<std::uint32_t>(pc.duration_ms);
      TraceEntry st;
      st.fields["type"] = "animation_start";
      st.fields["epoch"] = epoch_;
      st.fields["animation"] = pc.animation;
      st.seq = pc.seq;
      trace_.push_back(st);
    } else {
      command_queue_.push_back(pc);
    }
    if (c.animation == "revive") care_stage_ = "content";
  }
}

void AppEngine::complete_sync(const std::string& patch_json) {
  if (!request_in_flight_) return;
  std::string body = server_.merge_patch(patch_json, clock_.utc_ms);
  JsonDocument patch;
  deserializeJson(patch, patch_json);
  JsonDocument raw;
  deserializeJson(raw, body);
  if (!patch["commands"].is<JsonArrayConst>()) raw["commands"] = JsonArray();
  if (!patch["event_results"].is<JsonArrayConst>()) raw["event_results"] = JsonArray();
  if (!patch["request_id"].is<const char*>()) raw["request_id"] = pending_request_id_;
  serializeJson(raw, body);
  if (raw["epoch"].is<const char*>()) {
    const std::string ep = raw["epoch"].as<std::string>();
    if (!epoch_.empty() && !reset_pending_ && ep != epoch_) {
      emit_trace("response_rejected", {{"reason", "epoch"}});
      request_in_flight_ = false;
      return;
    }
  }
  auto parsed = network::parse_sync_response(body, pending_request_id_, epoch_, state_version_);
  if (!parsed) {
    emit_trace("response_rejected", {{"reason", "parse"}});
    request_in_flight_ = false;
    return;
  }
  if (!parsed->ok) {
    emit_trace("response_rejected", {{"reason", parsed->reject_reason}});
    request_in_flight_ = false;
    return;
  }
  const std::size_t capacity = 4 - (active_command_.has_value() ? 1 : 0);
  for (std::size_t i = 1; i < parsed->commands.size(); ++i) {
    if (parsed->commands[i].seq < parsed->commands[i - 1].seq) {
      emit_trace("response_rejected", {{"reason", "descending"}});
      request_in_flight_ = false;
      return;
    }
  }
  if (parsed->commands.size() > capacity) {
    emit_trace("response_rejected", {{"reason", "queue_overflow"}});
    request_in_flight_ = false;
    return;
  }
  if (parsed->state_version > state_version_) state_version_ = parsed->state_version;
  if (!parsed->epoch.empty()) {
    epoch_ = parsed->epoch;
    paired_ = true;
    reset_pending_ = false;
  }
  demo_mode_ = parsed->demo_mode;
  next_sync_ms_ = parsed->next_sync_ms;
  if (!parsed->care_stage.empty()) care_stage_ = parsed->care_stage;
  if (demo_mode_) last_demo_sync_ms_ = clock_.utc_ms;
  const bool ghost = care_stage_ == "ghost";
  apply_event_results(parsed->event_results);
  apply_commands(parsed->commands, ghost);
  request_in_flight_ = false;
  net_ = NetStatus::Ready;
  next_sync_at_ms_ = clock_.millis32() + static_cast<std::uint32_t>(next_sync_ms_);
  sync_due_ = false;
  if (pending_demo_intent_.has_value() && demo_fresh()) {
    const auto route = *pending_demo_intent_;
    pending_demo_intent_.reset();
    if (route.action == core::RoutedAction::DemoFood || route.action == core::RoutedAction::DemoRide ||
        route.action == core::RoutedAction::DemoSavings) {
      network::OutboxEvent e;
      e.event_id = new_uuid();
      e.type = "demo_trigger";
      e.payload_json = "{\"scenario\":\"" + route.scenario + "\"}";
      enqueue_event(e);
    }
  }
}

void AppEngine::complete_http(int status, const std::string& body) {
  if (!request_in_flight_ && status != 401) try_send_sync();
  if (!request_in_flight_) return;
  if (status == 401) {
    unauthorized_ = true;
    net_ = NetStatus::Unauthorized;
    request_in_flight_ = false;
    return;
  }
  if (status == 409) {
    reset_pending_ = true;
    paired_ = true;
    epoch_.clear();
    ack_seq_ = 0;
    outbox_.clear();
    command_queue_.clear();
    active_command_.reset();
    active_animation_ = "idle";
    server_.set_epoch("22222222-2222-4222-8222-222222222222");
    request_in_flight_ = false;
    persist_state();
    return;
  }
  if (status >= 500 || status == 503) {
    net_ = NetStatus::Backoff;
    request_in_flight_ = false;
    next_sync_at_ms_ = clock_.millis32() + 2000;
    return;
  }
  complete_sync(body);
}

void AppEngine::handle_nfc() {
  core::NfcTagRead read;
  if (!nfc_.consume_presentation(&read)) return;
  nfc::ParseResult parsed = read.user_memory.empty() ? nfc::ParseResult::Unrelated
                                                     : nfc::parse_type2_user_memory(read.user_memory);
  const bool fresh = demo_fresh();
  if (demo_mode_ && online_ && !fresh && (parsed == nfc::ParseResult::Food || parsed == nfc::ParseResult::Ride ||
                                          parsed == nfc::ParseResult::Savings)) {
    core::RouteDecision pending;
    if (parsed == nfc::ParseResult::Food) {
      pending.action = core::RoutedAction::DemoFood;
      pending.scenario = "food";
    } else if (parsed == nfc::ParseResult::Ride) {
      pending.action = core::RoutedAction::DemoRide;
      pending.scenario = "ride";
    } else {
      pending.action = core::RoutedAction::DemoSavings;
      pending.scenario = "savings";
    }
    pending_demo_intent_ = pending;
    pending_intent_deadline_ms_ = clock_.millis32() + 10000;
    return;
  }
  auto route = core::route_ndef(parsed, demo_mode_, online_, fresh);
  if (route.action == core::RoutedAction::None) return;
  if (route.action == core::RoutedAction::DemoFood || route.action == core::RoutedAction::DemoRide ||
      route.action == core::RoutedAction::DemoSavings) {
    network::OutboxEvent e;
    e.event_id = new_uuid();
    e.type = "demo_trigger";
    e.payload_json = "{\"scenario\":\"" + route.scenario + "\"}";
    enqueue_event(e);
  } else {
    network::OutboxEvent e;
    e.event_id = new_uuid();
    e.type = "review_requested";
    e.payload_json = "{\"source\":\"" + route.review_source + "\"}";
    enqueue_event(e);
  }
}

void AppEngine::pump_once() {
  buttons_.advance(clock_.monotonic_ms);
  handle_buttons();
  nfc_.advance(5);
  handle_nfc();
  if (pending_demo_intent_.has_value() && demo_fresh()) {
    const auto route = *pending_demo_intent_;
    pending_demo_intent_.reset();
    if (route.action == core::RoutedAction::DemoFood || route.action == core::RoutedAction::DemoRide ||
        route.action == core::RoutedAction::DemoSavings) {
      network::OutboxEvent e;
      e.event_id = new_uuid();
      e.type = "demo_trigger";
      e.payload_json = "{\"scenario\":\"" + route.scenario + "\"}";
      enqueue_event(e);
    }
  }
  if (active_command_.has_value() && clock_.millis32() >= active_anim_end_ms_) {
    active_command_.reset();
    if (!command_queue_.empty()) {
      auto pc = command_queue_.front();
      command_queue_.erase(command_queue_.begin());
      active_command_ = pc;
      active_animation_ = pc.animation;
      active_anim_end_ms_ = clock_.millis32() + static_cast<std::uint32_t>(pc.duration_ms);
      TraceEntry st;
      st.fields["type"] = "animation_start";
      st.fields["epoch"] = epoch_;
      st.fields["animation"] = pc.animation;
      st.seq = pc.seq;
      trace_.push_back(st);
    } else {
      active_animation_ = "idle";
    }
  }
  try_send_sync();
}

void AppEngine::pump(std::uint64_t until_ms) {
  while (clock_.monotonic_ms < until_ms) {
    const std::uint32_t step = static_cast<std::uint32_t>(std::min<std::uint64_t>(5, until_ms - clock_.monotonic_ms));
    clock_.advance(step);
    pump_once();
  }
}

void AppEngine::process_step_json(const std::string& step_json) {
  JsonDocument step;
  deserializeJson(step, step_json);
  const char* op = step["op"];
  if (!op) return;
  if (std::strcmp(op, "advance") == 0) {
    const std::uint64_t target = clock_.monotonic_ms + step["ms"].as<std::uint32_t>();
    pump(target);
  } else if (std::strcmp(op, "button") == 0) {
    const char* name = step["name"];
    const bool down = step["down"].as<bool>();
    core::ButtonId id = core::ButtonId::A;
    if (name && name[0] == 'B') id = core::ButtonId::B;
    if (name && name[0] == 'C') id = core::ButtonId::C;
    buttons_.set_raw(id, down);
    pump_once();
  } else if (std::strcmp(op, "nfc") == 0) {
    if (step["observation"].is<const char*>() && std::strcmp(step["observation"], "error") == 0) {
      nfc_.set_error();
    } else if (step["present"].is<bool>()) {
      nfc_.set_present(step["present"].as<bool>());
      if (step["ndef_hex"].is<const char*>()) {
        const std::string hex = step["ndef_hex"].as<std::string>();
        std::vector<std::uint8_t> mem;
        for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
          const auto byte = static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
          mem.push_back(byte);
        }
        nfc_.set_tag_memory(mem);
      }
    }
    pump_once();
  } else if (std::strcmp(op, "connectivity") == 0) {
    online_ = step["online"].as<bool>();
    if (!online_ && request_in_flight_) {
      request_in_flight_ = false;
      net_ = NetStatus::Backoff;
    }
  } else if (std::strcmp(op, "sync") == 0) {
    try_send_sync();
    std::string patch;
    if (step["patch"].is<JsonObjectConst>()) serializeJson(step["patch"], patch);
    complete_sync(patch);
  } else if (std::strcmp(op, "http") == 0) {
    try_send_sync();
    const int status = step["status"].as<int>();
    std::string body;
    if (step["body_fixture"].is<const char*>()) {
      const char* fx = step["body_fixture"];
      std::ifstream in(std::string("tests/fixtures/") + (std::strcmp(fx, "reset") == 0 ? "reset-error.json"
                                                                                       : "unauthorized-error.json"));
      std::ostringstream ss;
      ss << in.rdbuf();
      body = ss.str();
    } else if (step["body"].is<const char*>()) {
      body = step["body"].as<std::string>();
    } else if (step["body"].is<JsonObjectConst>()) {
      serializeJson(step["body"], body);
    }
    complete_http(status, body);
  } else if (std::strcmp(op, "enqueue") == 0) {
    JsonObject ev = step["event"];
    network::OutboxEvent e;
    e.event_id = ev["event_id"].as<std::string>();
    e.type = ev["type"].as<std::string>();
    serializeJson(ev["payload"], e.payload_json);
    enqueue_event(e);
  } else if (std::strcmp(op, "fail_store") == 0) {
    store_.failures_remaining = step["writes"].is<int>() ? step["writes"].as<unsigned>() : 1;
  } else if (std::strcmp(op, "reboot") == 0) {
    reboot();
  }
}

std::string AppEngine::run_steps(const std::string& steps_json) {
  JsonDocument steps;
  deserializeJson(steps, steps_json);
  for (JsonObject s : steps.as<JsonArray>()) {
    std::string one;
    serializeJson(s, one);
    process_step_json(one);
  }
  return "{}";
}

std::string AppEngine::snapshot_json() const {
  JsonDocument doc;
  doc["page"] = page_name();
  doc["active_animation"] = active_animation_;
  doc["care_stage"] = care_stage_;
  doc["ack_command_seq"] = ack_seq_;
  doc["outbox_size"] = outbox_.size();
  if (epoch_.empty()) doc["epoch"] = nullptr;
  else doc["epoch"] = epoch_;
  doc["state_version"] = state_version_;
  doc["storage_fault"] = storage_fault_;
  doc["network_status"] = net_name();
  doc["queued_commands"] = command_queue_.size();
  doc["mute"] = mute_;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string AppEngine::trace_json() const {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& t : trace_) {
    JsonObject o = arr.add<JsonObject>();
    for (const auto& kv : t.fields) o[kv.first] = kv.second;
    if (t.seq) o["seq"] = t.seq;
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string AppEngine::requests_json() const {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& r : requests_) {
    JsonDocument req;
    deserializeJson(req, r);
    arr.add(req.as<JsonVariantConst>());
  }
  std::string out;
  serializeJson(doc, out);
  return out;
}

} // namespace okanegachi::app
