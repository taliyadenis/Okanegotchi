#pragma once
#ifdef ARDUINO
#include "../engine.hpp"
#include <SPI.h>
namespace okanegachi::platform {
// The loop task owns SPI, including both this reader and the LCD.
// ACK/response waits are state transitions, never a busy wait.
class Pn532 {
public:
  void begin(SPIClass &, int cs);
  void tick(uint64_t now, Engine &);
  const char *status() const { return status_; }

private:
  SPIClass *spi_{};
  int cs_{};
  uint64_t deadline_{}, next_{};
  uint8_t command_{};
  enum Phase { Idle, Ack, Response };
  Phase phase_{Idle};
  enum Job { Version, Sam, Rf, Scan, TagVersion, Capability, Read };
  Job job_{Version};
  bool detected_{};
  uint8_t page_{4}, cc_size_{0x12};
  uint8_t memory_[144]{};
  const char *status_{"starting"};
  void select();
  void release();
  bool ready();
  void send(Job, uint8_t, const uint8_t *, size_t, uint64_t);
  void data_exchange(Job, uint8_t, uint8_t, uint64_t);
  int read_frame(uint8_t *);
  void fail(uint64_t, Engine &);
  void received(const uint8_t *, size_t, uint64_t, Engine &);
};
} // namespace okanegachi::platform
#endif
