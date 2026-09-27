#ifdef ARDUINO
#include "../Okanegachi.h"
#include "../assets.hpp"
#include "../board.hpp"
#include "../demo.hpp"
#include "pn532.hpp"
#include "services.hpp"
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <algorithm>
#include <memory>

#ifndef OKANEGACHI_LCD_ROTATION
#define OKANEGACHI_LCD_ROTATION 0
#endif
namespace okanegachi {
namespace {
using namespace platform;
struct DeviceClock : SystemClock {
  int64_t utc() const override {
    return local_demo ? 1790359200000LL + int64_t(now()) : SystemClock::utc();
  }
} clock_;
SystemRandom random_;
NvsStore storage;
Config config;
Network network;
Pn532 reader;
std::unique_ptr<Engine> engine;
std::unique_ptr<DemoServer> demo;
Adafruit_ST7789 lcd(&SPI, pins::LCD_CS, pins::LCD_DC, pins::LCD_RST);
bool configured{}, network_ready{}, tone_ready{};
uint64_t wifi_retry{}, frame_due{}, tone_until{}, all_held_since{};
std::string serial_line;
bool serial_overflow{};
uint64_t serial_since{};
// USB bench input exists only in LOCAL DEMO. Deadlines release each simulated
// button even if the test client disconnects; physical reset consent is separate.
uint64_t demo_button_until[3]{};
uint16_t tile[240 * 16];
int strip = 240;
Appearance frame_pet;
std::string frame_animation;
uint64_t frame_elapsed{};

// A logical 240x240 GFX canvas backed by one 16-row tile. Clipped primitives
// let the same UI render in slices; SPI is released between every slice.
class Canvas : public Adafruit_GFX {
public:
  int top{};
  Canvas() : Adafruit_GFX(240, 240) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x >= 0 && x < 240 && y >= top && y < top + 16)
      tile[(y - top) * 240 + x] = color;
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                uint16_t color) override {
    int right = std::min<int>(240, x + w),
        bottom = std::min<int>(top + 16, y + h);
    for (int yy = std::max<int>(top, y); yy < bottom; ++yy)
      if (right > std::max<int>(0, x))
        std::fill(tile + (yy - top) * 240 + std::max<int>(0, x),
                  tile + (yy - top) * 240 + right, color);
  }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) override {
    fillRect(x, y, w, 1, c);
  }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) override {
    fillRect(x, y, 1, h, c);
  }
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t c) override {
    fillRect(x, y, w, h, c);
  }
  void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) override {
    fillRect(x, y, w, 1, c);
  }
  void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) override {
    fillRect(x, y, 1, h, c);
  }
} canvas;
void text(int x, int y, const std::string &value, int size = 1,
          uint16_t color = 0xffff) {
  canvas.setCursor(x, y);
  canvas.setTextSize(size);
  canvas.setTextColor(color);
  canvas.setTextWrap(false);
  canvas.print(display_text(value, size == 1 ? 38 : 19).c_str());
}
void money_line(int y, const char *label, int value) {
  text(8, y, std::string(label) + money(value), 2);
}
void goal(const std::optional<Goal> &g, int y) {
  if (!g) {
    text(8, y, "No savings goal set", 2);
    return;
  }
  text(8, y, g->name, 2);
  text(8, y + 25, money(g->saved) + " / " + money(g->target));
  canvas.drawRect(8, y + 45, 224, 12, 0x7bef);
  int width = int(
      std::min<int64_t>(222, int64_t(g->saved) * 222 / std::max(1, g->target)));
  canvas.fillRect(9, y + 46, width, 10, 0x5db2);
}
void ui() {
  const auto &e = *engine;
  const auto &s = e.state();
  auto page = e.page();
  canvas.fillRect(0, 0, 240, 25, 0x1084);
  text(5, 5,
       local_demo   ? "LOCAL DEMO"
       : configured ? e.status()
                    : "USB SETUP",
       1, local_demo ? 0xffe0 : 0x7fff);
  text(162, 5, e.persistent().mute ? "MUTE" : "SOUND");
  if (page == Page::Pet) {
    text(8, 30, e.appearance().name, 2, 0x0000);
    text(8, 51, e.animation(), 1, 0x0000);
  } else if (page == Page::Goal) {
    text(8, 32, "Savings goal", 2);
    goal(s.goal, 65);
    text(8, 168, "Goal transfers are not expenses.");
  } else if (page == Page::CheckIn) {
    text(8, 32, "Check-in", 2);
    text(8, 66, "AM: " + std::string(s.am ? "complete" : "pending"), 2);
    text(8, 93, "PM: " + std::string(s.pm ? "complete" : "pending"), 2);
    text(8, 130, "Streak: " + std::to_string(s.streak) + " days");
    text(8, 154, "B: review your spending");
    text(8, 174, "Offline actions await confirmation.");
  } else if (page == Page::Review) {
    text(8, 32, "Spending review", 2);
    const auto &r = e.displayed_review();
    if (r) {
      money_line(60, "Spent ", r->finance.spend);
      text(8, 86,
           r->finance.budget ? "Budget " + money(*r->finance.budget)
                             : "No budget set");
      text(8, 104,
           "Snapshot " + std::to_string(r->version) + " (" + r->finance.source +
               ")");
      goal(r->goal, 126);
      text(8, 190, "Hold B 1s: confirm this review");
    } else
      text(8, 90, "B: request a fresh review");
  } else if (page == Page::Menu) {
    text(8, 32, "Menu", 2);
    const char *items[] = {"Review spending", "Catch a star", "Connection",
                           "Toggle sound"};
    for (int i = 0; i < 4; ++i)
      text(8, 65 + i * 28,
           std::string(i == e.menu_index() ? "> " : "  ") + items[i], 1,
           i == e.menu_index() ? 0xffe0 : 0xffff);
    text(8, 182, "A/C: choose   B: open");
  } else if (page == Page::Connection) {
    text(8, 32, "Connection", 2);
    text(8, 62,
         configured ? "Wi-Fi: " + std::string(WiFi.status() == WL_CONNECTED
                                                  ? "connected"
                                                  : "reconnecting")
                    : "Run tools/provision.py");
    text(8, 82, "NFC: " + std::string(reader.status()));
    text(8, 102,
         "Pending actions: " + std::to_string(e.persistent().outbox.size()));
    text(8, 122, "Free heap: " + std::to_string(ESP.getFreeHeap()));
    text(8, 142, "PSRAM: " + std::to_string(ESP.getPsramSize()));
    text(8, 172, "B: retry   Hold A: home");
  } else if (page == Page::Game) {
    const auto &g = e.game();
    text(8, 32, "Catch a star", 2);
    text(8, 53,
         "Score " + std::to_string(g.score) + "  " +
             std::to_string(g.played_ms / 1000) + "/15s");
    for (int x : {80, 160})
      canvas.drawFastVLine(x, 74, 110, 0x7bef);
    int star_y = 78 + int(g.played_ms % 1000) * 95 / 1000;
    canvas.fillRect(g.star_lane * 80 + 35, star_y, 10, 10, 0xffe0);
    canvas.fillRect(g.lane * 80 + 20, 187, 40, 8, 0x5db2);
    text(8, 202,
         g.paused    ? "Paused - B resumes"
         : g.started ? "A/C: move    B: pause"
                     : "B: start    Hold A: exit");
  }
  canvas.fillRect(0, 218, 240, 22, 0x1084);
  text(5, 221,
       configured || local_demo ? e.notice() : "Connect USB; run provision.py");
  text(5, 232,
       page == Page::Pet ? (s.stage == "ghost" ? "Hold B: review to revive"
                                               : "Hold B: review  Hold C: play")
                         : "Hold A: home");
}
void draw(uint64_t now) {
  if (strip >= 240) {
    if (now < frame_due)
      return;
    strip = 0;
    frame_due = now + 83;
    frame_pet = engine->appearance();
    frame_animation = engine->animation();
    frame_elapsed = engine->animation_elapsed();
  }
  if (engine->page() == Page::Pet)
    assets::render(tile, strip, 16, frame_pet, frame_animation, frame_elapsed);
  else
    std::fill(std::begin(tile), std::end(tile), uint16_t(0x1084));
  canvas.top = strip;
  ui();
  lcd.drawRGBBitmap(0, strip, tile, 240, 16);
  strip += 16;
}
void status() {
  JsonDocument d;
  d["firmware"] = "0.2.0";
  d["mode"] = local_demo ? "LOCAL_DEMO" : "CLOUD";
  d["configured"] = configured;
  d["status"] = engine->status();
  d["nfc"] = reader.status();
  d["heap_free"] = ESP.getFreeHeap();
  d["heap_min"] = ESP.getMinFreeHeap();
  d["psram_bytes"] = ESP.getPsramSize();
  d["flash_bytes"] = ESP.getFlashChipSize();
  d["pending"] = engine->persistent().outbox.size();
  d["uptime_ms"] = clock_.now();
  if (local_demo) {
    const auto &s = engine->state();
    const auto &g = engine->game();
    const char *pages[] = {"pet", "goal", "checkin", "menu", "review", "game",
                           "connection"};
    auto info = d["demo"].to<JsonObject>();
    info["page"] = pages[static_cast<int>(engine->page())];
    info["animation"] = engine->animation();
    info["pet"] = engine->appearance().pet;
    info["care_stage"] = s.stage;
    info["spend_minor"] = s.finance.spend;
    if (s.goal)
      info["saved_minor"] = s.goal->saved;
    else
      info["saved_minor"] = nullptr;
    info["am_complete"] = s.am;
    info["pm_complete"] = s.pm;
    info["ack_command_seq"] = engine->persistent().ack;
    info["queued_commands"] = engine->queued();
    info["mute"] = engine->persistent().mute;
    info["storage_fault"] = engine->storage_fault();
    info["review_available"] = bool(engine->displayed_review());
    info["game_started"] = g.started;
    info["game_paused"] = g.paused;
    info["game_lane"] = g.lane;
    info["notice"] = engine->notice();
  }
  serializeJson(d, Serial);
  Serial.println();
}
void serial_command(const std::string &raw) {
  JsonDocument d;
  if (deserializeJson(d, raw, DeserializationOption::NestingLimit(3))) {
    Serial.println("{\"ok\":false,\"error\":\"invalid_json\"}");
    return;
  }
  auto op = d["op"].as<std::string>();
  if (op == "status") {
    status();
    return;
  }
  if (local_demo && op == "demo_button") {
    auto name = d["button"].as<std::string>();
    int index = name == "A" ? 0 : name == "B" ? 1 : name == "C" ? 2 : -1;
    if (index < 0 || !d["duration_ms"].is<int>() ||
        d["duration_ms"].as<int>() < 80 ||
        d["duration_ms"].as<int>() > 2000) {
      Serial.println("{\"ok\":false,\"error\":\"invalid_demo_button\"}");
      return;
    }
    demo_button_until[index] = clock_.now() + d["duration_ms"].as<int>();
    Serial.println("{\"ok\":true}");
    return;
  }
  if (op == "configure") {
    if (local_demo) {
      Serial.println("{\"ok\":false,\"error\":\"flash_cloud_build_first\"}");
      return;
    }
    Config next{d["ssid"] | "", d["password"] | "", d["url"] | "",
                d["device_id"] | "", d["device_token"] | ""};
    if (save_config(next)) {
      Serial.println("{\"ok\":true,\"restarting\":true}");
      Serial.flush();
      ESP.restart();
    } else
      Serial.println("{\"ok\":false,\"error\":\"invalid_config_or_storage\"}");
    return;
  }
  if (op == "factory_reset") {
    if (!all_held_since || clock_.now() - all_held_since < 2000) {
      Serial.println("{\"ok\":false,\"error\":\"hold_all_three_buttons_2s\"}");
      return;
    }
    Preferences p;
    bool ok = p.begin("oka-config", false) && p.clear();
    p.end();
    ok = p.begin(local_demo ? "oka-local" : "oka-live", false) && p.clear() &&
         ok;
    p.end();
    Serial.println(ok ? "{\"ok\":true}" : "{\"ok\":false}");
    if (ok) {
      Serial.flush();
      ESP.restart();
    }
    return;
  }
  if (local_demo && op == "demo") {
    auto scenario = d["scenario"].as<std::string>();
    bool ok = demo->scenario(scenario);
    if (ok)
      engine->manual_retry();
    Serial.println(ok ? "{\"ok\":true}" : "{\"ok\":false}");
    return;
  }
  if (local_demo && op == "pet") {
    bool ok = demo->choose_pet(d["id"] | "");
    if (ok)
      engine->manual_retry();
    Serial.println(ok ? "{\"ok\":true}" : "{\"ok\":false}");
    return;
  }
  Serial.println("{\"ok\":false,\"error\":\"unknown_command\"}");
}
void serial_tick(uint64_t now) {
  if (!serial_line.empty() && now - serial_since > 5000) {
    serial_line.clear();
    serial_overflow = true;
  }
  int budget = 64;
  while (budget-- && Serial.available()) {
    char c = char(Serial.read());
    if (c == '\r')
      continue;
    if (c == '\n') {
      if (!serial_overflow && !serial_line.empty())
        serial_command(serial_line);
      else if (serial_overflow)
        Serial.println("{\"ok\":false,\"error\":\"line_limit\"}");
      std::fill(serial_line.begin(), serial_line.end(), '\0');
      serial_line.clear();
      serial_overflow = false;
    } else if (!serial_overflow) {
      if (serial_line.empty())
        serial_since = now;
      if (serial_line.size() >= 1024) {
        serial_overflow = true;
        serial_line.clear();
      } else
        serial_line.push_back(c);
    }
  }
}
} // namespace
void setup_device() {
  Serial.begin(115200);
  serial_line.reserve(1024);
  for (int pin : {pins::A, pins::B, pins::C})
    pinMode(pin, INPUT_PULLUP);
  pinMode(pins::PIEZO, OUTPUT);
  digitalWrite(pins::PIEZO, LOW);
  tone_ready = ledcAttach(pins::PIEZO, 2000, 8);
  pinMode(pins::LCD_CS, OUTPUT);
  pinMode(pins::NFC_CS, OUTPUT);
  digitalWrite(pins::LCD_CS, HIGH);
  digitalWrite(pins::NFC_CS, HIGH);
  SPI.begin(pins::SCK, pins::MISO, pins::MOSI);
  lcd.init(240, 240, SPI_MODE0);
  lcd.setSPISpeed(20000000);
  lcd.setRotation(OKANEGACHI_LCD_ROTATION);
  reader.begin(SPI, pins::NFC_CS);
  configured = load_config(config);
  storage.begin();
  if (local_demo) {
    config.device = "00000000-0000-4000-8000-000000000001";
    demo = std::make_unique<DemoServer>(clock_, random_);
  }
  engine = std::make_unique<Engine>(
      storage, clock_, random_,
      configured || local_demo ? config.device
                               : "00000000-0000-4000-8000-000000000000",
      local_demo   ? "local-demo"
      : configured ? binding(config)
                   : "unconfigured");
  engine->begin();
  engine->boot_button(core::ButtonId::A, digitalRead(pins::A) == LOW);
  engine->boot_button(core::ButtonId::B, digitalRead(pins::B) == LOW);
  engine->boot_button(core::ButtonId::C, digitalRead(pins::C) == LOW);
  if (!local_demo && configured) {
    network_ready = network.begin(config);
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    configTime(0, 0, "time.cloudflare.com", "pool.ntp.org");
    wifi_retry = clock_.now() + 15000;
  }
  status();
}
void loop_device() {
  auto now = clock_.now();
  bool a = digitalRead(pins::A) == LOW, b = digitalRead(pins::B) == LOW,
       c = digitalRead(pins::C) == LOW;
  if (a && b && c) {
    if (!all_held_since)
      all_held_since = now;
  } else
    all_held_since = 0;
  if (local_demo) {
    a = a || now < demo_button_until[0];
    b = b || now < demo_button_until[1];
    c = c || now < demo_button_until[2];
  }
  engine->button(core::ButtonId::A, a);
  engine->button(core::ButtonId::B, b);
  engine->button(core::ButtonId::C, c);
  engine->tick();
  serial_tick(now);
  bool connected = local_demo || (configured && network_ready &&
                                  WiFi.status() == WL_CONNECTED &&
                                  clock_.utc() >= 1735689600000LL);
  engine->connectivity(connected);
  if (!local_demo && configured && WiFi.status() != WL_CONNECTED &&
      now >= wifi_retry) {
    WiFi.reconnect();
    wifi_retry = now + 15000;
  }
  if (auto result = network.receive())
    engine->complete(result->id, result->status, result->body,
                     result->retry_ms);
  if (!network.busy())
    if (auto req = engine->next_request(local_demo ? -40 : WiFi.RSSI())) {
      if (local_demo) {
        auto result = demo->sync(*req);
        engine->complete(req->id, result.first, result.second);
      } else if (!network.send(*req))
        engine->complete(req->id, 0, "");
    }
  reader.tick(now, *engine);
  int sound = engine->take_tone();
  if (tone_ready) {
    if (engine->persistent().mute) {
      ledcWriteTone(pins::PIEZO, 0);
      tone_until = 0;
    } else if (sound) {
      ledcWriteTone(pins::PIEZO, sound == 2 ? 2400 : sound == 3 ? 400 : 1600);
      tone_until = now + (sound == 2 ? 180 : 70);
    } else if (tone_until && now >= tone_until) {
      ledcWriteTone(pins::PIEZO, 0);
      tone_until = 0;
    }
  }
  draw(now);
  delay(1); // Scheduler yield, not animation timing.
}
} // namespace okanegachi
#endif
