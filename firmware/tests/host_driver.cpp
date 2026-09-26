#include "assets.hpp"
#include "demo.hpp"
#include "engine.hpp"
#include <iostream>
#include <memory>
#include <sstream>

using namespace okanegachi;
struct MemoryStore : Store {
  std::string data;
  bool fail{}, corrupt{};
  int writes{};
  Load read(std::string &out) override {
    out = data;
    return corrupt ? Load::Corrupt : data.empty() ? Load::Missing : Load::Ok;
  }
  bool write(const std::string &s) override {
    if (fail)
      return false;
    data = s;
    ++writes;
    return true;
  }
};
struct FakeClock : Clock {
  uint64_t ms{};
  uint64_t now() const override { return ms; }
  int64_t utc() const override { return 1790359200000LL + ms; }
};
struct FakeRandom : Random {
  unsigned counter{};
  std::string uuid() override {
    char s[40];
    snprintf(s, sizeof(s), "00000000-0000-4000-8000-%012u", ++counter);
    return s;
  }
  uint32_t jitter(uint32_t max) override { return counter % (max + 1); }
};
MemoryStore store;
FakeClock clock_;
FakeRandom random_;
std::unique_ptr<Engine> engine;
std::unique_ptr<DemoServer> demo;
std::vector<Trace> traces;
void reboot() {
  engine = std::make_unique<Engine>(store, clock_, random_,
                                    "00000000-0000-4000-8000-999999999999",
                                    "host-test");
  engine->trace = [](const Trace &t) { traces.push_back(t); };
  engine->begin();
}
void advance(uint64_t ms) {
  auto end = clock_.ms + ms;
  while (clock_.ms < end) {
    clock_.ms = std::min(end, clock_.ms + 5);
    engine->tick();
  }
}
int main() {
  reboot();
  std::string line;
  while (std::getline(std::cin, line)) {
    JsonDocument in, out;
    if (deserializeJson(in, line)) {
      std::cout << "{\"error\":\"json\"}\n";
      continue;
    }
    auto op = in["op"].as<std::string>();
    traces.clear();
    if (op == "reset") {
      store = MemoryStore{};
      clock_ = FakeClock{};
      random_ = FakeRandom{};
      demo.reset();
      reboot();
    } else if (op == "reboot")
      reboot();
    else if (op == "online")
      engine->connectivity(in["value"] | false);
    else if (op == "advance")
      advance(in["ms"] | 0ULL);
    else if (op == "storage_fail")
      store.fail = in["value"] | false;
    else if (op == "corrupt") {
      store.corrupt = true;
      reboot();
    } else if (op == "button") {
      auto b = in["id"].as<std::string>();
      auto id = b == "A"   ? core::ButtonId::A
                : b == "B" ? core::ButtonId::B
                           : core::ButtonId::C;
      if (in["boot"] | false)
        engine->boot_button(id, in["down"] | false);
      else
        engine->button(id, in["down"] | false);
      engine->tick();
    } else if (op == "next") {
      auto req = engine->next_request(-42);
      if (req) {
        JsonDocument d;
        deserializeJson(d, req->body);
        out["request"] = d;
      } else
        out["request"] = nullptr;
    } else if (op == "complete") {
      std::string body;
      if (in["body"].is<const char *>())
        body = in["body"].as<std::string>();
      else
        serializeJson(in["body"], body);
      out["accepted"] =
          engine->complete(in["id"].as<std::string>(), in["status"] | 200, body,
                           in["retry_ms"] | 0ULL);
    } else if (op == "review")
      out["accepted"] = engine->review("button");
    else if (op == "retry")
      engine->manual_retry();
    else if (op == "demo_start")
      demo = std::make_unique<DemoServer>(clock_, random_);
    else if (op == "demo_scenario")
      out["accepted"] = demo && demo->scenario(in["scenario"] | "");
    else if (op == "demo_pet")
      out["accepted"] = demo && demo->choose_pet(in["pet"] | "");
    else if (op == "demo_sync") {
      auto request = engine->next_request(-40);
      if (request && demo) {
        auto response = demo->sync(*request);
        out["http_status"] = response.first;
        out["accepted"] =
            engine->complete(request->id, response.first, response.second);
        JsonDocument body;
        deserializeJson(body, response.second);
        out["response"] = body;
      } else
        out["accepted"] = false;
    } else if (op == "enqueue") {
      std::string payload;
      serializeJson(in["payload"], payload);
      out["accepted"] = engine->enqueue(
          {in["id"].as<std::string>(), in["type"].as<std::string>(), payload});
    } else if (op == "nfc") {
      auto state = in["state"].as<std::string>();
      auto parsed = in["parsed"].as<std::string>();
      auto p = parsed == "food"      ? nfc::ParseResult::Food
               : parsed == "ride"    ? nfc::ParseResult::Ride
               : parsed == "savings" ? nfc::ParseResult::Savings
               : parsed == "review"  ? nfc::ParseResult::Review
               : parsed == "invalid" ? nfc::ParseResult::Invalid
                                     : nfc::ParseResult::Unrelated;
      engine->nfc(state == "absent"  ? Observation::Absent
                  : state == "error" ? Observation::Error
                                     : Observation::Present,
                  p);
    } else if (op == "validate") {
      std::string body;
      if (in["body"].is<const char *>())
        body = in["body"].as<std::string>();
      else
        serializeJson(in["body"], body);
      auto schema = in["schema"].as<std::string>();
      out["valid"] =
          validate_json(body, schema == "request" ? Schema::Request
                              : schema == "error" ? Schema::Error
                                                  : Schema::Response);
    } else if (op == "parse_nfc") {
      std::string hex = in["hex"].as<std::string>();
      std::vector<uint8_t> b;
      for (size_t i = 0; i + 1 < hex.size(); i += 2)
        b.push_back(uint8_t(std::stoul(hex.substr(i, 2), nullptr, 16)));
      out["parsed"] = nfc::parse_result_name(nfc::parse_type2_user_memory(b));
    } else if (op == "retry_header") {
      out["delay_ms"] =
          retry_after_ms(in["value"] | "", *utc_millis("2026-09-25T18:30:05Z"));
    } else if (op == "sample_clip") {
      std::vector<assets::Frame> frames(in["durations_ms"].size());
      std::vector<assets::Step> steps;
      for (size_t i = 0; i < frames.size(); ++i)
        steps.push_back({&frames[i], in["durations_ms"][i].as<uint32_t>()});
      assets::Clip clip{"test", steps.data(), steps.size(),
                        in["mode"] == "loop"};
      out["frame"] =
          int(&assets::frame_at(clip, in["elapsed_ms"].as<uint64_t>()) -
              frames.data());
    } else if (op == "render") {
      Appearance a;
      a.pet = in["pet"] | "gator";
      a.palette = in["palette"] | "original";
      a.accessory = in["accessory"] | "none";
      uint16_t tile[240 * 16];
      uint64_t hash = 1469598103934665603ULL;
      std::string pixels;
      bool capture = in["pixels"] | false;
      if (capture) {
        pixels.reserve(240 * 16 * 4);
        out["pixels"].to<JsonArray>();
      }
      for (int y = 0; y < 240; y += 16) {
        assets::render(tile, y, 16, a, in["animation"] | "idle",
                       in["ms"] | 0ULL);
        for (auto p : tile) {
          hash ^= p;
          hash *= 1099511628211ULL;
          if (capture) {
            const char *hex = "0123456789abcdef";
            for (int bit : {12, 8, 4, 0})
              pixels.push_back(hex[(p >> bit) & 15]);
          }
        }
        if(capture){out["pixels"].as<JsonArray>().add(pixels);pixels.clear();}
      }
      out["hash"] = std::to_string(hash);
      out["pixel_bytes"] = assets::pixel_bytes;
    }
    out["status"] = engine->status();
    out["notice"] = engine->notice();
    out["ack"] = engine->persistent().ack;
    out["epoch"] = engine->persistent().epoch;
    out["queued"] = engine->queued();
    out["animation"] = engine->animation();
    out["page"] = int(engine->page());
    out["version"] = engine->persistent().version;
    out["stage"] = engine->state().stage;
    out["outbox_count"] = engine->persistent().outbox.size();
    out["storage_fault"] = engine->storage_fault();
    out["writes"] = store.writes;
    out["score"] = engine->game().score;
    out["spend"] = engine->state().finance.spend;
    out["pet"] = engine->appearance().pet;
    auto events = out["outbox"].to<JsonArray>();
    for (auto &e : engine->persistent().outbox) {
      auto v = events.add<JsonObject>();
      v["id"] = e.id;
      v["type"] = e.type;
      JsonDocument payload;
      deserializeJson(payload, e.payload);
      v["payload"] = payload;
    }
    auto trace = out["trace"].to<JsonArray>();
    for (auto &t : traces) {
      auto v = trace.add<JsonObject>();
      v["type"] = t.type;
      v["detail"] = t.detail;
      v["seq"] = t.seq;
    }
    serializeJson(out, std::cout);
    std::cout << std::endl;
  }
}
