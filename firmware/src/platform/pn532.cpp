#ifdef ARDUINO
#include "pn532.hpp"
#include <cstring>

namespace okanegachi::platform {
void Pn532::select() {
  spi_->beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
  digitalWrite(cs_, LOW);
  delayMicroseconds(10);
}
void Pn532::release() {
  digitalWrite(cs_, HIGH);
  spi_->endTransaction();
}
void Pn532::begin(SPIClass &spi, int cs) {
  spi_ = &spi;
  cs_ = cs;
  pinMode(cs_, OUTPUT);
  digitalWrite(cs_, LOW);
  delay(3);
  digitalWrite(cs_, HIGH);
  next_ = 0;
}
bool Pn532::ready() {
  select();
  spi_->transfer(0x02);
  auto b = spi_->transfer(0);
  release();
  return b == 1;
}
void Pn532::send(Job job, uint8_t command, const uint8_t *args, size_t size,
                 uint64_t now) {
  job_ = job;
  command_ = command;
  uint8_t length = uint8_t(size + 2);
  uint8_t sum = uint8_t(0xD4 + command);
  select();
  spi_->transfer(1);
  spi_->transfer(0);
  spi_->transfer(0);
  spi_->transfer(0xFF);
  spi_->transfer(length);
  spi_->transfer(uint8_t(-length));
  spi_->transfer(0xD4);
  spi_->transfer(command);
  for (size_t i = 0; i < size; ++i) {
    spi_->transfer(args[i]);
    sum += args[i];
  }
  spi_->transfer(uint8_t(-sum));
  spi_->transfer(0);
  release();
  phase_ = Ack;
  deadline_ = now + 1000;
}
void Pn532::data_exchange(Job job, uint8_t instruction, uint8_t page,
                          uint64_t now) {
  uint8_t a[] = {1, instruction, page};
  send(job, 0x40, a, instruction == 0x60 ? 2 : 3, now);
}
int Pn532::read_frame(uint8_t *out) {
  select();
  spi_->transfer(3);
  uint8_t h[5];
  for (auto &b : h)
    b = spi_->transfer(0);
  if (h[0] != 0 || h[1] != 0 || h[2] != 0xff || uint8_t(h[3] + h[4]) != 0) {
    release();
    return -1;
  }
  if (h[3] < 2 || h[3] > 62) {
    release();
    return -1;
  }
  uint8_t sum = 0;
  for (unsigned i = 0; i < h[3]; ++i) {
    out[i] = spi_->transfer(0);
    sum += out[i];
  }
  sum += spi_->transfer(0);
  auto post = spi_->transfer(0);
  release();
  if (sum || post || out[0] != 0xD5 || out[1] != uint8_t(command_ + 1))
    return -1;
  return h[3];
}
void Pn532::fail(uint64_t now, Engine &engine) {
  // Host ACK aborts an outstanding command before retrying initialization.
  const uint8_t ack[] = {0, 0, 0xff, 0, 0xff, 0};
  select();
  spi_->transfer(1);
  for (auto b : ack)
    spi_->transfer(b);
  release();
  engine.nfc(Observation::Error);
  phase_ = Idle;
  job_ = Version;
  next_ = now + 3000;
  status_ = "reader unavailable";
}
void Pn532::received(const uint8_t *p, size_t n, uint64_t now, Engine &engine) {
  switch (job_) {
  case Version: {
    if (n < 4 || p[0] != 0x32) {
      fail(now, engine);
      return;
    }
    uint8_t a[] = {1, 0x14, 0};
    send(Sam, 0x14, a, 3, now);
    return;
  }
  case Sam: {
    uint8_t a[] = {5, 0xff, 1, 1};
    send(Rf, 0x32, a, 4, now);
    return;
  }
  case Rf:
    status_ = "ready";
    phase_ = Idle;
    job_ = Scan;
    next_ = now;
    return;
  case Scan:
    if (n < 1) {
      fail(now, engine);
      return;
    }
    if (p[0] == 0) {
      detected_ = false;
      engine.nfc(Observation::Absent);
      break;
    }
    if (p[0] != 1 || n < 6 || p[1] != 1 || p[5] > 10 || n < size_t(6 + p[5])) {
      fail(now, engine);
      return;
    }
    if (detected_) {
      engine.nfc(Observation::Present);
      break;
    }
    detected_ = true;
    // InListPassiveTarget: count, target, SENS_RES[2], SEL_RES, UID length.
    // Never infer card type from UID. ISO-DEP/payment cards go straight to
    // Review.
    if (p[2] == 0x00 && p[3] == 0x44 && p[4] == 0x00) {
      data_exchange(TagVersion, 0x60, 0, now);
      return;
    }
    engine.nfc(Observation::Present, nfc::ParseResult::Unrelated);
    break;
  case TagVersion:
    // Only identified NTAG213/215/216 receive Type 2 READ commands. For all
    // three, read only the first 144 user bytes (pages 4..39), never config.
    if (n != 9 || p[0] != 0 || p[1] != 0 || p[2] != 4 || p[3] != 4 ||
        p[4] != 2 || p[5] != 1 || p[6] != 0 ||
        (p[7] != 0x0f && p[7] != 0x11 && p[7] != 0x13) || p[8] != 3) {
      engine.nfc(Observation::Present, nfc::ParseResult::Unrelated);
      break;
    }
    cc_size_ = p[7] == 0x0f ? 0x12 : p[7] == 0x11 ? 0x3e : 0x6d;
    data_exchange(Capability, 0x30, 3, now);
    return;
  case Capability:
    if (n != 17 || p[0] != 0 || p[1] != 0xE1 || (p[2] >> 4) != 1 ||
        p[3] != cc_size_ || (p[4] >> 4) != 0) {
      engine.nfc(Observation::Present, nfc::ParseResult::Invalid);
      break;
    }
    page_ = 4;
    data_exchange(Read, 0x30, page_, now);
    return;
  case Read:
    if (n != 17 || p[0] != 0) {
      engine.nfc(Observation::Present, nfc::ParseResult::Invalid);
      break;
    }
    std::memcpy(memory_ + (page_ - 4) * 4, p + 1, 16);
    if (page_ < 36) {
      page_ += 4;
      data_exchange(Read, 0x30, page_, now);
      return;
    }
    engine.nfc(Observation::Present,
               nfc::parse_type2_user_memory(memory_, sizeof(memory_)));
    break;
  }
  phase_ = Idle;
  job_ = Scan;
  next_ = now + 80;
}
void Pn532::tick(uint64_t now, Engine &engine) {
  if (phase_ == Idle) {
    if (now < next_)
      return;
    if (job_ == Version)
      send(Version, 2, nullptr, 0, now);
    else {
      uint8_t a[] = {1, 0};
      send(Scan, 0x4a, a, 2, now);
    }
    return;
  }
  if (now >= deadline_) {
    fail(now, engine);
    return;
  }
  if (!ready())
    return;
  uint8_t data[64]{};
  // ACK is six bytes and uses 00 ff length rather than the data frame checksum.
  if (phase_ == Ack) {
    select();
    spi_->transfer(3);
    const uint8_t ack[] = {0, 0, 0xff, 0, 0xff, 0};
    bool valid = true;
    for (auto b : ack)
      if (spi_->transfer(0) != b)
        valid = false;
    release();
    if (!valid) {
      fail(now, engine);
      return;
    }
    phase_ = Response;
    deadline_ = now + 1200;
    return;
  }
  auto length = read_frame(data);
  if (length < 2) {
    fail(now, engine);
    return;
  }
  received(data + 2, size_t(length - 2), now, engine);
}
} // namespace okanegachi::platform
#endif
