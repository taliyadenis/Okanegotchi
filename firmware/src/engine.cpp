#include "engine.hpp"
#include <algorithm>
#include <set>

namespace okanegachi {
std::string encode_persistent(const Persistent &p) {
  JsonDocument d;
  d["format"] = 1;
  d["binding"] = p.binding;
  d["epoch"] = p.epoch;
  d["ack"] = p.ack;
  d["version"] = p.version;
  d["mute"] = p.mute;
  d["cache"] = p.cached_response;
  auto a = d["events"].to<JsonArray>();
  for (const auto &e : p.outbox) {
    auto v = a.add<JsonObject>();
    v["id"] = e.id;
    v["type"] = e.type;
    v["payload"] = e.payload;
  }
  std::string out;
  serializeJson(d, out);
  return out;
}
std::optional<Persistent> decode_persistent(const std::string &s) {
  if (s.size() > 16384)
    return {};
  JsonDocument d;
  if (deserializeJson(d, s, DeserializationOption::NestingLimit(5)))
    return {};
  if (d["format"] != 1 || !d["binding"].is<const char *>() ||
      !d["epoch"].is<const char *>() || !d["ack"].is<int>() ||
      !d["version"].is<int>() || !d["mute"].is<bool>() ||
      !d["cache"].is<const char *>() || !d["events"].is<JsonArray>())
    return {};
  Persistent p;
  p.binding = d["binding"].as<std::string>();
  p.epoch = d["epoch"].as<std::string>();
  p.ack = d["ack"];
  p.version = d["version"];
  p.mute = d["mute"];
  p.cached_response = d["cache"].as<std::string>();
  if (p.binding.size() > 128 || p.ack < 0 || p.version < 0 ||
      (!p.epoch.empty() && !valid_uuid(p.epoch)) || d["events"].size() > 8)
    return {};
  for (auto v : d["events"].as<JsonArrayConst>()) {
    if (!v["id"].is<const char *>() || !v["type"].is<const char *>() ||
        !v["payload"].is<const char *>())
      return {};
    p.outbox.push_back({v["id"].as<std::string>(), v["type"].as<std::string>(),
                        v["payload"].as<std::string>()});
  }
  if (p.epoch.empty() && (p.ack || !p.outbox.empty()))
    return {};
  if (!p.cached_response.empty()) {
    auto r = decode_response(p.cached_response);
    if (!r || r->epoch != p.epoch || r->version > p.version)
      return {};
  }
  if (!p.epoch.empty() && encode_request("00000000-0000-4000-8000-000000000001",
                                         "00000000-0000-4000-8000-000000000002",
                                         p.epoch, p.ack, p.outbox, 0, -127)
                              .empty())
    return {};
  std::set<std::string> ids;
  for (auto &e : p.outbox)
    if (!ids.insert(e.id).second)
      return {};
  return p;
}
Engine::Engine(Store &s, Clock &c, Random &r, std::string device,
               std::string binding)
    : store_(s), clock_(c), random_(r), device_(std::move(device)),
      binding_(std::move(binding)) {
  buttons_.set_trace([this](const core::ButtonTrace &e) { on_button(e); });
}
void Engine::emit(std::string t, std::string d, std::string id, int seq) {
  if (trace)
    trace({std::move(t), std::move(d), std::move(id), seq});
}
bool Engine::begin() {
  saved_ = Persistent{};
  saved_.binding = binding_;
  buttons_.sync_time(clock_.now());
  std::string raw;
  auto result = store_.read(raw);
  if (result == Load::Corrupt) {
    storage_fault_ = true;
    notice_ = "Storage fault";
    status_ = "storage_fault";
    return false;
  }
  if (result == Load::Ok) {
    auto p = decode_persistent(raw);
    if (!p) {
      storage_fault_ = true;
      status_ = "storage_fault";
      notice_ = "Storage record invalid";
      return false;
    }
    if (p->binding == binding_)
      saved_ = std::move(*p);
    else if (!commit(saved_))
      return false;
  }
  if (!saved_.cached_response.empty()) {
    auto r = decode_response(saved_.cached_response);
    state_ = r->state;
  }
  visible_appearance_ = state_.appearance;
  notice_ = saved_.epoch.empty() ? "Ready to connect" : "Cached data";
  return true;
}
bool Engine::commit(const Persistent &candidate) {
  const auto data = encode_persistent(candidate);
  if (data.size() > 16384 || !store_.write(data)) {
    storage_fault_ = true;
    status_ = "storage_fault";
    notice_ = "Storage fault";
    intent_.clear();
    emit("storage_error");
    return false;
  }
  saved_ = candidate;
  return true;
}
int64_t Engine::utc_now() const {
  return have_success_ ? anchor_utc_ + int64_t(clock_.now() - anchor_mono_)
                       : clock_.utc();
}
bool Engine::demo_fresh() const {
  return connected_ && !blocked_ && !storage_fault_ && have_success_ && demo_ &&
         clock_.now() - last_success_ <= 10000;
}
void Engine::connectivity(bool connected) {
  if (connected_ == connected)
    return;
  connected_ = connected;
  if (!connected) {
    status_ = "offline";
    intent_.clear();
  } else {
    dirty_ = true;
    status_ = "connecting";
  }
}
void Engine::backoff(const std::string &why, uint64_t minimum) {
  static const uint32_t delays[] = {2000, 4000, 8000, 16000, 30000, 60000};
  const auto base = delays[std::min<unsigned>(failures_, 5)];
  if (failures_ < 6)
    ++failures_;
  not_before_ = clock_.now() +
                std::max<uint64_t>(minimum, base + random_.jitter(base / 5));
  dirty_ = true;
  status_ = "backoff";
  notice_ = why;
  intent_.clear();
}
std::optional<Request> Engine::next_request(int rssi) {
  auto now = clock_.now();
  if (!connected_ || blocked_ || storage_fault_ || in_flight_ ||
      now < not_before_ || (sent_before_ && now - last_send_ < 1000) ||
      (!dirty_ && now < next_poll_))
    return {};
  request_.id = random_.uuid();
  request_.body = encode_request(request_.id, device_, saved_.epoch, saved_.ack,
                                 saved_.outbox, now / 1000, rssi);
  if (request_.body.empty()) {
    blocked_ = true;
    status_ = "error";
    notice_ = "Request contract error";
    return {};
  }
  submitted_.clear();
  if (!saved_.epoch.empty())
    for (auto &e : saved_.outbox)
      submitted_.push_back(e.id);
  in_flight_ = true;
  dirty_ = false;
  last_send_ = now;
  sent_before_ = true;
  return request_;
}
bool Engine::complete(const std::string &id, int status,
                      const std::string &body, uint64_t retry) {
  if (!in_flight_ || id != request_.id)
    return false;
  in_flight_ = false;
  if (status == 401 || status == 403 || status == 426) {
    blocked_ = true;
    intent_.clear();
    status_ = status == 401   ? "unauthorized"
              : status == 403 ? "forbidden"
                              : "update_required";
    notice_ = status == 426 ? "Update required" : "Reprovision device";
    return false;
  }
  if (status == 409) {
    if (!validate_json(body, Schema::Error)) {
      backoff("Invalid reset response");
      return false;
    }
    JsonDocument d;
    deserializeJson(d, body);
    if (d["request_id"] != id || d["error"]["code"] != "RESET_REQUIRED") {
      backoff("Invalid reset identity");
      return false;
    }
    Persistent fresh;
    fresh.binding = binding_;
    fresh.mute = saved_.mute;
    if (!commit(fresh))
      return false;
    pending_.clear();
    active_.reset();
    displayed_review_.reset();
    intent_.clear();
    state_ = State{};
    visible_appearance_ = state_.appearance;
    have_success_ = false;
    demo_ = false;
    dirty_ = true;
    next_poll_ = not_before_ = clock_.now();
    page_ = Page::Pet;
    notice_ = "Resetting demo";
    return true;
  }
  if (status == 400 || status == 422) {
    blocked_ = true;
    status_ = "error";
    notice_ = "Request rejected; check firmware";
    intent_.clear();
    return false;
  }
  if (status != 200) {
    if (status == 429 && validate_json(body, Schema::Error)) {
      JsonDocument d;
      deserializeJson(d, body);
      if (d["request_id"] == id && d["error"]["code"] == "RATE_LIMITED")
        retry = std::max<uint64_t>(retry,
                                   d["error"]["retry_after_ms"].as<uint64_t>());
    }
    backoff(status == 429 ? "Server rate limit" : "Connection failed", retry);
    return false;
  }
  auto r = decode_response(body);
  auto reject = [this](const char *why) {
    emit("response_rejected", why);
    backoff(why);
    return false;
  };
  if (!r)
    return reject("Invalid response");
  if (r->request_id != id)
    return reject("request_id");
  const bool bootstrap = saved_.epoch.empty();
  if ((!bootstrap && r->epoch != saved_.epoch) ||
      (bootstrap && !r->commands.empty()))
    return reject("epoch/bootstrap");
  if (!bootstrap && (r->version < saved_.version ||
                     r->state.finance.version < state_.finance.version))
    return reject("stale_state");
  std::set<std::string> result_ids;
  for (auto &e : r->results)
    if (!result_ids.insert(e.id).second ||
        std::find(submitted_.begin(), submitted_.end(), e.id) ==
            submitted_.end())
      return reject("unsolicited_result");
  std::vector<Command> fresh;
  int previous = 0;
  std::set<std::string> command_ids;
  for (auto &c : r->commands) {
    if (c.seq <= previous || !command_ids.insert(c.id).second)
      return reject("command_order");
    previous = c.seq;
    if (c.animation == "revive" && r->state.stage == "ghost")
      return reject("revive_state");
    if (active_ && c.seq == active_->seq && !(c == *active_))
      return reject("command_conflict");
    auto existing = std::find_if(pending_.begin(), pending_.end(),
                                 [&](auto &x) { return x.seq == c.seq; });
    if (existing != pending_.end()) {
      if (!(c == *existing))
        return reject("command_conflict");
      continue;
    }
    if (c.seq <= saved_.ack)
      continue;
    if (active_ && active_->id == c.id)
      return reject("command_identity");
    for (auto &x : pending_)
      if (x.id == c.id)
        return reject("command_identity");
    fresh.push_back(c);
  }
  size_t total = pending_.size() + fresh.size();
  if (!active_ && total)
    --total;
  if (total > 4)
    return reject("queue_overflow");
  Persistent candidate = saved_;
  candidate.epoch = r->epoch;
  candidate.version = r->version;
  for (auto &e : r->results)
    candidate.outbox.erase(
        std::remove_if(candidate.outbox.begin(), candidate.outbox.end(),
                       [&](auto &x) { return x.id == e.id; }),
        candidate.outbox.end());
  // Cache meaningful state changes; volatile poll timestamps do not cause
  // writes.
  if (bootstrap || r->version != saved_.version ||
      saved_.cached_response.empty()) {
    JsonDocument cached;
    deserializeJson(cached, body);
    cached["commands"].to<JsonArray>();
    cached["event_results"].to<JsonArray>();
    candidate.cached_response.clear();
    serializeJson(cached, candidate.cached_response);
  }
  if (encode_persistent(candidate) != encode_persistent(saved_) &&
      !commit(candidate))
    return false;
  state_ = r->state;
  demo_ = r->demo;
  anchor_utc_ = *utc_millis(r->server_time);
  anchor_mono_ = last_success_ = clock_.now();
  have_success_ = true;
  failures_ = 0;
  not_before_ = 0;
  next_poll_ = clock_.now() + r->poll_ms;
  status_ = "ready";
  if (state_.stage == "ghost") {
    active_.reset();
    if (page_ == Page::Game)
      page_ = Page::Pet;
  }
  pending_.insert(pending_.end(), fresh.begin(), fresh.end());
  std::sort(pending_.begin(), pending_.end(),
            [](auto &a, auto &b) { return a.seq < b.seq; });
  for (auto &e : r->results) {
    bool checkin = false;
    JsonDocument request;
    deserializeJson(request, request_.body);
    for (auto event : request["events"].as<JsonArrayConst>())
      if (event["event_id"] == e.id && event["type"] == "checkin_completed")
        checkin = true;
    if (e.status == "rejected") {
      notice_ = e.reason;
      tone_ = 3;
    } else {
      notice_ = checkin ? "Check-in confirmed" : "Synced";
      tone_ = 2;
    }
    if (checkin) {
      displayed_review_.reset();
      page_ = Page::CheckIn;
    }
  }
  if (state_.review && (!displayed_review_ || page_ != Page::Review)) {
    bool requested = false;
    for (auto &e : r->results)
      if (e.status != "rejected") {
        JsonDocument req;
        deserializeJson(req, request_.body);
        for (auto event : req["events"].as<JsonArrayConst>())
          if (event["event_id"] == e.id && event["type"] == "review_requested")
            requested = true;
      }
    if (requested) {
      displayed_review_ = state_.review;
      page_ = Page::Review;
      notice_ = "Hold B to confirm";
    }
  }
  if (!intent_.empty()) {
    auto intent = intent_;
    intent_.clear();
    if (clock_.now() < intent_deadline_) {
      if (demo_fresh())
        action("demo_trigger", "{\"scenario\":\"" + intent + "\"}");
      else if (connected_ && !demo_)
        review("sticker");
    }
  }
  start_next();
  if (!active_)
    visible_appearance_ = state_.appearance;
  // Events created after this request retain their immediate-send intent.
  for (auto &e : saved_.outbox)
    if (std::find(submitted_.begin(), submitted_.end(), e.id) ==
        submitted_.end())
      dirty_ = true;
  if (bootstrap)
    dirty_ = true;
  return true;
}
bool Engine::enqueue(const Event &e) {
  if (storage_fault_ || saved_.epoch.empty()) {
    notice_ = "Connect before this action";
    return false;
  }
  for (auto &x : saved_.outbox)
    if (x.id == e.id)
      return x.type == e.type && x.payload == e.payload;
  if (saved_.outbox.size() >= 8) {
    notice_ = "Pending actions full";
    tone_ = 3;
    return false;
  }
  auto p = saved_;
  p.outbox.push_back(e);
  if (encode_request("00000000-0000-4000-8000-000000000001", device_, p.epoch,
                     p.ack, p.outbox, 0, -127)
          .empty())
    return false;
  if (!commit(p))
    return false;
  emit("event_queued", e.type, e.id);
  dirty_ = true;
  notice_ = "Pending sync";
  tone_ = 1;
  return true;
}
bool Engine::action(const char *type, const std::string &payload) {
  return enqueue({random_.uuid(), type, payload});
}
bool Engine::review(const char *source) {
  return action("review_requested",
                "{\"source\":\"" + std::string(source) + "\"}");
}
bool Engine::consume(int seq) {
  auto p = saved_;
  p.ack = seq;
  if (!commit(p))
    return false;
  emit("cursor_saved", saved_.epoch, {}, seq);
  return true;
}
void Engine::start_next() {
  if (active_ || storage_fault_)
    return;
  while (!pending_.empty()) {
    const auto c = pending_.front();
    if (!consume(c.seq))
      return;
    pending_.erase(pending_.begin());
    auto expiry = utc_millis(c.expires);
    if (!expiry || *expiry <= utc_now() || state_.stage == "ghost")
      continue;
    active_ = c;
    active_start_ = clock_.now();
    visible_appearance_ = state_.appearance;
    // Reactions return to the pet, except while reading a frozen receipt.
    // Revival after a confirmed review must be visible immediately.
    if (page_ != Page::Review || c.animation == "revive")
      page_ = Page::Pet;
    emit("animation_start", c.animation, saved_.epoch, c.seq);
    tone_ = c.animation == "celebrate" || c.animation == "revive" ? 2 : 1;
    return;
  }
  visible_appearance_ = state_.appearance;
}
void Engine::tick() {
  auto now = clock_.now();
  buttons_.advance(now);
  if (!intent_.empty() && now >= intent_deadline_) {
    intent_.clear();
    notice_ = "Tap expired; tap again";
  }
  if (active_ && now - active_start_ >= uint64_t(active_->duration)) {
    active_.reset();
    start_next();
  }
  if (!active_)
    visible_appearance_ = state_.appearance;
  if (page_ == Page::Game) {
    uint64_t delta = now - game_.last_tick;
    game_.last_tick = now;
    if (game_.started && !game_.paused) {
      game_.played_ms = std::min<uint64_t>(15000, game_.played_ms + delta);
      int target = int(game_.played_ms / 1000);
      while (game_.dropped < target) {
        if (game_.lane == game_.star_lane)
          ++game_.score;
        ++game_.dropped;
        game_.star_lane = int(random_.jitter(2));
      }
      if (game_.played_ms >= 15000) {
        game_.started = false;
        page_ = Page::Pet;
        action("play_completed", "{\"game\":\"catch_star\",\"score\":" +
                                     std::to_string(game_.score) + "}");
        local_animation_ = "pat";
        local_start_ = now;
        local_end_ = now + 1500;
      }
    }
  }
}
void Engine::nfc(Observation o, nfc::ParseResult p) {
  auto now = clock_.now();
  if (o == Observation::Error) {
    absence_started_ = false;
    return;
  }
  if (o == Observation::Absent) {
    if (!absence_started_) {
      absence_started_ = true;
      absence_start_ = now;
    } else if (now - absence_start_ >= 500)
      nfc_armed_ = true;
    return;
  }
  absence_started_ = false;
  if (!nfc_armed_)
    return;
  nfc_armed_ = false;
  tone_ = 1;
  if (p == nfc::ParseResult::Invalid) {
    notice_ = "Unrecognized sticker";
    return;
  }
  if (p == nfc::ParseResult::Food || p == nfc::ParseResult::Ride ||
      p == nfc::ParseResult::Savings) {
    if (!connected_ || blocked_) {
      notice_ = "Connect for live demo";
      return;
    }
    const char *scenario = p == nfc::ParseResult::Food   ? "food"
                           : p == nfc::ParseResult::Ride ? "ride"
                                                         : "savings";
    if (demo_fresh())
      action("demo_trigger",
             "{\"scenario\":\"" + std::string(scenario) + "\"}");
    else if (have_success_ && !demo_)
      review("sticker");
    else {
      intent_ = scenario;
      intent_deadline_ = now + 10000;
      dirty_ = true;
      notice_ = "Checking demo session";
    }
  } else
    review(p == nfc::ParseResult::Review ? "sticker" : "card");
}
void Engine::navigate(int d) {
  if (page_ == Page::Menu) {
    menu_index_ = (menu_index_ + d + 4) % 4;
    return;
  }
  int page = page_ == Page::Pet       ? 0
             : page_ == Page::Goal    ? 1
             : page_ == Page::CheckIn ? 2
                                      : 3;
  page = (page + d + 4) % 4;
  page_ = static_cast<Page>(page);
}
void Engine::start_game() {
  if (state_.stage == "ghost") {
    notice_ = "Review to revive";
    return;
  }
  game_ = Game{};
  game_.last_tick = clock_.now();
  game_.star_lane = int(random_.jitter(2));
  page_ = Page::Game;
}
void Engine::confirm_review() {
  if (!displayed_review_) {
    notice_ = "Request a review first";
    return;
  }
  auto expiry = utc_millis(displayed_review_->expires);
  if (!have_success_ || !expiry || *expiry <= utc_now()) {
    notice_ = "Review expired; request again";
    return;
  }
  for (auto &e : saved_.outbox)
    if (e.type == "checkin_completed") {
      notice_ = "Confirmation pending";
      return;
    }
  action("checkin_completed",
         "{\"review_id\":\"" + displayed_review_->id + "\"}");
}
void Engine::select() {
  if (page_ == Page::Pet) {
    if (state_.stage != "ghost") {
      local_animation_ = "pat";
      local_start_ = clock_.now();
      local_end_ = local_start_ + 1000;
      tone_ = 1;
    }
  } else if (page_ == Page::CheckIn)
    review("button");
  else if (page_ == Page::Review) {
    if (!displayed_review_ || !utc_millis(displayed_review_->expires) ||
        *utc_millis(displayed_review_->expires) <= utc_now())
      review("button");
    else
      notice_ = "Hold B to confirm";
  } else if (page_ == Page::Menu) {
    if (menu_index_ == 0)
      review("button");
    else if (menu_index_ == 1)
      start_game();
    else if (menu_index_ == 2)
      page_ = Page::Connection;
    else {
      auto p = saved_;
      p.mute = !p.mute;
      commit(p);
    }
  } else if (page_ == Page::Connection)
    manual_retry();
}
void Engine::on_button(const core::ButtonTrace &e) {
  emit(e.type, e.button);
  bool hold = e.type == "button_long";
  if (hold && e.button == "A") {
    page_ = Page::Pet;
    return;
  }
  if (page_ == Page::Game) {
    if (!hold) {
      if (e.button == "A")
        game_.lane = std::max(0, game_.lane - 1);
      else if (e.button == "C")
        game_.lane = std::min(2, game_.lane + 1);
      else {
        if (!game_.started) {
          game_.started = true;
          game_.last_tick = clock_.now();
        } else
          game_.paused = !game_.paused;
      }
    }
    return;
  }
  if (hold) {
    if (e.button == "B") {
      if (page_ == Page::Pet)
        review("button");
      else if (page_ == Page::Review)
        confirm_review();
    } else if (e.button == "C" && page_ == Page::Pet)
      start_game();
  } else if (e.button == "A")
    navigate(-1);
  else if (e.button == "C")
    navigate(1);
  else
    select();
}
void Engine::manual_retry() {
  if (blocked_ || storage_fault_) {
    notice_ = "USB setup or firmware repair needed";
    return;
  }
  dirty_ = true;
  notice_ = "Retry requested";
}
std::string Engine::animation() const {
  if (state_.stage == "ghost")
    return "ghost";
  if (active_)
    return active_->animation;
  if (clock_.now() < local_end_)
    return local_animation_;
  return state_.stage == "content" ? "idle" : state_.stage;
}
uint64_t Engine::animation_elapsed() const {
  return clock_.now() - (active_                     ? active_start_
                         : clock_.now() < local_end_ ? local_start_
                                                     : 0);
}
} // namespace okanegachi
