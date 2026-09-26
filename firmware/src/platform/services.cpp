#ifdef ARDUINO
#include "services.hpp"
#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <esp_random.h>
#include <esp_timer.h>
#include <mbedtls/sha256.h>

namespace okanegachi::platform {
uint64_t SystemClock::now() const {
  return uint64_t(esp_timer_get_time() / 1000);
}
int64_t SystemClock::utc() const { return int64_t(time(nullptr)) * 1000; }
std::string SystemRandom::uuid() {
  uint8_t b[16];
  esp_fill_random(b, sizeof(b));
  b[6] = (b[6] & 15) | 0x40;
  b[8] = (b[8] & 63) | 0x80;
  char s[37];
  snprintf(
      s, sizeof(s),
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
  return s;
}
uint32_t SystemRandom::jitter(uint32_t max) {
  return max == UINT32_MAX ? esp_random() : esp_random() % (max + 1);
}
bool valid_config(const Config &c) {
  if (c.ssid.empty() || c.ssid.size() > 32 || c.password.size() > 63 ||
      (!c.password.empty() && c.password.size() < 8) || !valid_uuid(c.device) ||
      c.token.size() < 32 || c.token.size() > 256)
    return false;
  const std::string prefix = "https://",
                    suffix = ".supabase.co/functions/v1/device-api/v1/sync";
  if (c.url.size() != prefix.size() + 20 + suffix.size() ||
      c.url.substr(0, prefix.size()) != prefix ||
      c.url.substr(prefix.size() + 20) != suffix)
    return false;
  for (size_t i = prefix.size(); i < prefix.size() + 20; ++i)
    if (!((c.url[i] >= 'a' && c.url[i] <= 'z') ||
          (c.url[i] >= '0' && c.url[i] <= '9')))
      return false;
  for (auto b : c.token)
    if (!((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') ||
          (b >= '0' && b <= '9') || b == '_' || b == '-'))
      return false;
  for (auto *s : {&c.ssid, &c.password})
    if (s->find('\0') != s->npos)
      return false;
  return true;
}
bool load_config(Config &c) {
  Preferences p;
  if (!p.begin("oka-config", true))
    return false;
  auto raw = p.getString("config", "");
  p.end();
  if (raw.length() > 1024)
    return false;
  JsonDocument d;
  if (deserializeJson(d, raw))
    return false;
  c = {d["ssid"] | "", d["password"] | "", d["url"] | "", d["device_id"] | "",
       d["device_token"] | ""};
  return valid_config(c);
}
bool save_config(const Config &c) {
  if (!valid_config(c))
    return false;
  JsonDocument d;
  d["ssid"] = c.ssid;
  d["password"] = c.password;
  d["url"] = c.url;
  d["device_id"] = c.device;
  d["device_token"] = c.token;
  std::string raw;
  serializeJson(d, raw);
  Preferences p;
  if (!p.begin("oka-config", false))
    return false;
  auto size = p.putString("config", raw.c_str());
  p.end();
  return size == raw.size();
}
std::string binding(const Config &c) {
  auto source = c.url + "|" + c.device + "|" + c.token;
  uint8_t hash[32];
  mbedtls_sha256(reinterpret_cast<const uint8_t *>(source.data()),
                 source.size(), hash, 0);
  char out[65];
  for (int i = 0; i < 32; ++i)
    snprintf(out + i * 2, 3, "%02x", hash[i]);
  return out;
}
namespace {
uint32_t crc32(const uint8_t *p, size_t n) {
  uint32_t c = 0xffffffff;
  while (n--) {
    c ^= *p++;
    for (int i = 0; i < 8; ++i)
      c = (c >> 1) ^ (0xedb88320U & uint32_t(-int(c & 1)));
  }
  return ~c;
}
} // namespace
bool NvsStore::begin() {
  ready_ = prefs_.begin(local_demo ? "oka-local" : "oka-live", false);
  return ready_;
}
Load NvsStore::read(std::string &out) {
  if (!ready_)
    return Load::Corrupt;
  auto n = prefs_.getBytesLength("state");
  if (!n)
    return Load::Missing;
  if (n < 8 || n > 16388)
    return Load::Corrupt;
  std::string raw(n, '\0');
  if (prefs_.getBytes("state", raw.data(), n) != n)
    return Load::Corrupt;
  uint32_t crc;
  memcpy(&crc, raw.data(), 4);
  if (crc != crc32(reinterpret_cast<const uint8_t *>(raw.data() + 4), n - 4))
    return Load::Corrupt;
  out.assign(raw.data() + 4, n - 4);
  return Load::Ok;
}
bool NvsStore::write(const std::string &data) {
  if (!ready_ || data.size() > 16384)
    return false;
  std::string raw(data.size() + 4, '\0');
  auto crc = crc32(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  memcpy(raw.data(), &crc, 4);
  memcpy(raw.data() + 4, data.data(), data.size());
  // One NVS blob commit keeps epoch, cursor and outbox together across reset.
  if (prefs_.putBytes("state", raw.data(), raw.size()) != raw.size())
    return false;
  std::string verify;
  return read(verify) == Load::Ok && verify == data;
}
namespace {
class BoundedBody : public Stream {
public:
  std::string data;
  bool failed{};
  uint64_t deadline = uint64_t(esp_timer_get_time() / 1000) + 12000;
  BoundedBody() { data.reserve(2048); }
  size_t write(uint8_t b) override { return write(&b, 1); }
  size_t write(const uint8_t *bytes, size_t size) override {
    if (uint64_t(esp_timer_get_time() / 1000) > deadline ||
        size > kResponseLimit - data.size()) {
      failed = true;
      return 0;
    }
    data.append(reinterpret_cast<const char *>(bytes), size);
    return size;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};
HttpResult post(const Config &config, const Request &req) {
  HttpResult r;
  r.id = req.id;
  if (WiFi.status() != WL_CONNECTED || time(nullptr) < 1735689600)
    return r;
  NetworkClientSecure tls;
  tls.useBuiltinCACertBundle();
  tls.setHandshakeTimeout(8);
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  if (!http.begin(tls, config.url.c_str()))
    return r;
  const char *keys[] = {"Content-Type", "Retry-After"};
  http.collectHeaders(keys, 2);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", ("Bearer " + config.token).c_str());
  // HTTPClient's pointer overload is mutable despite not modifying payloads.
  // A worker-owned copy avoids const-casting the engine's request.
  std::string payload = req.body;
  r.status =
      http.POST(reinterpret_cast<uint8_t *>(payload.data()), payload.size());
  r.retry_ms = retry_after_ms(http.header("Retry-After").c_str(),
                              int64_t(time(nullptr)) * 1000);
  // Authentication/update failures must stop retries even if a gateway returns
  // a large HTML body or no body. These status codes need no JSON payload.
  const bool terminal_status=r.status==401||r.status==403||r.status==426||r.status==400||r.status==422;
  if (r.status > 0 && !terminal_status) {
    auto type = http.header("Content-Type");
    bool json =
        type == "application/json" || type.startsWith("application/json;");
    if (http.getSize() > int(kResponseLimit) || (r.status == 200 && !json)) {
      r.status = 0;
    } else {
      BoundedBody body;
      auto received = http.writeToStream(&body);
      if (received < 0 || body.failed)
        r.status = 0;
      else
        r.body = std::move(body.data);
    }
  }
  http.end();
  tls.stop();
  return r;
}
} // namespace
bool Network::begin(const Config &c) {
  config_ = c;
  requests_ = xQueueCreate(1, sizeof(Request *));
  results_ = xQueueCreate(1, sizeof(HttpResult *));
  if (!requests_ || !results_)
    return false;
  return xTaskCreatePinnedToCore(run, "oka-http", 12288, this, 1, nullptr, 0) ==
         pdPASS;
}
bool Network::send(const Request &request) {
  if (busy_)
    return false;
  auto p = new (std::nothrow) Request(request);
  if (!p)
    return false;
  if (xQueueSend(requests_, &p, 0) != pdTRUE) {
    delete p;
    return false;
  }
  busy_ = true;
  return true;
}
std::unique_ptr<HttpResult> Network::receive() {
  HttpResult *result = nullptr;
  if (!results_ || xQueueReceive(results_, &result, 0) != pdTRUE)
    return {};
  busy_ = false;
  return std::unique_ptr<HttpResult>(result);
}
void Network::run(void *self) {
  auto &n = *static_cast<Network *>(self);
  for (;;) {
    Request *p = nullptr;
    if (xQueueReceive(n.requests_, &p, portMAX_DELAY) != pdTRUE)
      continue;
    auto *result = new HttpResult(post(n.config_, *p));
    delete p;
    xQueueSend(n.results_, &result, portMAX_DELAY);
  }
}
} // namespace okanegachi::platform
#endif
