#pragma once
#ifdef ARDUINO
#include "../engine.hpp"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <memory>

#ifndef OKANEGACHI_LOCAL_DEMO
#define OKANEGACHI_LOCAL_DEMO 0
#endif
namespace okanegachi::platform {
inline constexpr bool local_demo = OKANEGACHI_LOCAL_DEMO;
struct Config {
  std::string ssid, password, url, device, token;
};
bool valid_config(const Config &);
bool load_config(Config &);
bool save_config(const Config &);
std::string binding(const Config &);
struct SystemClock : Clock {
  uint64_t now() const override;
  int64_t utc() const override;
};
struct SystemRandom : Random {
  std::string uuid() override;
  uint32_t jitter(uint32_t) override;
};
class NvsStore : public Store {
  Preferences prefs_;
  bool ready_{};

public:
  bool begin();
  Load read(std::string &) override;
  bool write(const std::string &) override;
};
struct HttpResult {
  std::string id, body;
  int status{};
  uint64_t retry_ms{};
};
class Network {
public:
  bool begin(const Config &);
  bool send(const Request &);
  std::unique_ptr<HttpResult> receive();
  bool busy() const { return busy_; }

private:
  Config config_;
  QueueHandle_t requests_{}, results_{};
  bool busy_{};
  static void run(void *);
};
} // namespace okanegachi::platform
#endif
