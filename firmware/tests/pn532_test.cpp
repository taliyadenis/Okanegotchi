#include "demo.hpp"
#include "platform/pn532.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace okanegachi;
struct Store_ : Store {
  std::string data;
  Load read(std::string &s) override {
    s = data;
    return data.empty() ? Load::Missing : Load::Ok;
  }
  bool write(const std::string &s) override {
    data = s;
    return true;
  }
};
struct Clock_ : Clock {
  uint64_t ms{};
  uint64_t now() const override { return ms; }
  int64_t utc() const override { return 1790359200000LL + ms; }
};
struct Random_ : Random {
  unsigned n{};
  std::string uuid() override {
    char s[37];
    snprintf(s, sizeof(s), "00000000-0000-4000-8000-%012u", ++n);
    return s;
  }
  uint32_t jitter(uint32_t) override { return 0; }
};
struct Rig {
  Store_ store;
  Clock_ clock;
  Random_ random;
  Engine engine{store, clock, random, "00000000-0000-4000-8000-000000000001",
                "pn-test"};
  DemoServer demo{clock, random};
  SPIClass spi;
  platform::Pn532 reader;
  bool present = true, payment = false, corrupt = false, timeout = false,
       invalid_cc = false, unknown = false;
  uint8_t size = 0x0f;
  int reads{}, versions{}, scans{};
  std::vector<int> pages;
  uint8_t memory[144]{};
  Rig() {
    const std::vector<uint8_t> text = {
        0x03, 0x1e, 0xd1, 1,   0x1a, 'T', 2,   'e', 'n', 'o', 'k',
        'a',  'n',  'e',  'g', 'a',  'c', 'h', 'i', ':', 'd', 'e',
        'm',  'o',  ':',  'v', '1',  ':', 'f', 'o', 'o', 'd', 0xfe};
    std::copy(text.begin(), text.end(), memory);
    engine.begin();
    engine.connectivity(true);
    auto req = engine.next_request();
    auto r = demo.sync(*req);
    assert(engine.complete(req->id, r.first, r.second));
    spi.command = [this](const std::vector<uint8_t> &b) {
      if (timeout)
        return;
      uint8_t cmd = b[6];
      std::vector<uint8_t> p;
      if (cmd == 2)
        p = {0x32, 1, 6, 7};
      else if (cmd == 0x14)
        assert(b[7] == 1 && b[9] == 0);
      else if (cmd == 0x32)
        assert(b[7] == 5 && b[10] == 1);
      else if (cmd == 0x4a) {
        ++scans;
        p = present ? std::vector<
                          uint8_t>{1, 1, 0, 0x44, uint8_t(payment ? 0x20 : 0),
                                   7, 4, 1, 2,    3,
                                   4, 5, 6}
                    : std::vector<uint8_t>{0};
      } else if (cmd == 0x40) {
        if (b[8] == 0x60) {
          ++versions;
          p = {0, 0, 4, uint8_t(unknown ? 5 : 4), 2, 1, 0, size, 3};
        } else {
          assert(b[8] == 0x30);
          ++reads;
          int page = b[9];
          pages.push_back(page);
          p.resize(17);
          if (page == 3) {
            p[1] = 0xe1;
            p[2] = 0x10;
            p[3] = invalid_cc     ? 0xff
                   : size == 0x0f ? 0x12
                   : size == 0x11 ? 0x3e
                                  : 0x6d;
          } else {
            assert(page >= 4 && page <= 36);
            std::copy(memory + (page - 4) * 4, memory + (page - 4) * 4 + 16,
                      p.begin() + 1);
          }
        }
      } else
        assert(false);
      spi.replies.push_back({0, 0, 255, 0, 255, 0});
      std::vector<uint8_t> frame = {0,
                                    0,
                                    255,
                                    uint8_t(p.size() + 2),
                                    uint8_t(-(p.size() + 2)),
                                    0xd5,
                                    uint8_t(cmd + 1)};
      frame.insert(frame.end(), p.begin(), p.end());
      uint8_t sum = 0;
      for (size_t i = 5; i < frame.size(); ++i)
        sum += frame[i];
      frame.push_back(uint8_t(-sum) + (corrupt ? 1 : 0));
      frame.push_back(0);
      spi.replies.push_back(frame);
    };
    reader.begin(spi, 1);
  }
  void advance(unsigned duration) {
    for (unsigned i = 0; i < duration; i += 5) {
      clock.ms += 5;
      engine.tick();
      reader.tick(clock.ms, engine);
    }
  }
};
int main() {
  for (uint8_t size : {0x0f, 0x11, 0x13}) {
    Rig r;
    r.size = size;
    r.advance(1000);
    assert(r.engine.persistent().outbox.size() == 1);
    assert(r.engine.persistent().outbox[0].type == "demo_trigger");
    assert(r.reads == 10 && r.pages.back() == 36);
    r.advance(500);
    assert(r.engine.persistent().outbox.size() == 1);
    r.present = false;
    r.advance(700);
    r.present = true;
    r.advance(700);
    assert(r.engine.persistent().outbox.size() == 2);
  }
  {
    Rig r;
    r.payment = true;
    r.advance(1000);
    assert(r.engine.persistent().outbox.size() == 1);
    assert(r.engine.persistent().outbox[0].type == "review_requested");
    assert(r.versions == 0 && r.reads == 0);
  }
  {
    Rig r;
    r.unknown = true;
    r.advance(1000);
    assert(r.engine.persistent().outbox.size() == 1 && r.reads == 0);
  }
  {
    Rig r;
    r.invalid_cc = true;
    r.advance(1000);
    assert(r.engine.persistent().outbox.empty() && r.reads == 1);
  }
  {
    Rig r;
    r.corrupt = true;
    r.advance(2000);
    assert(r.engine.persistent().outbox.empty());
    r.corrupt = false;
    r.advance(4000);
    assert(r.engine.persistent().outbox.size() == 1);
  }
  {
    Rig r;
    r.timeout = true;
    r.advance(2000);
    assert(r.engine.persistent().outbox.empty());
    r.timeout = false;
    r.advance(4000);
    assert(r.engine.persistent().outbox.size() == 1);
  }
  std::cout << "PASS 8 PN532 SPI transcript scenarios (NTAG213/215/216, "
               "held/removed tag, payment review, unknown tag, invalid CC, "
               "checksum failure, timeout recovery). Physical RF not tested.\n";
}
