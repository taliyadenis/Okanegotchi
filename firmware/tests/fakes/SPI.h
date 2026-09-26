#pragma once
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>
constexpr int LOW = 0, HIGH = 1, OUTPUT = 1, LSBFIRST = 0, SPI_MODE0 = 0;
inline void digitalWrite(int, int) {}
inline void pinMode(int, int) {}
inline void delay(int) {}
inline void delayMicroseconds(int) {}
struct SPISettings {
  SPISettings(unsigned speed, int order, int mode) {
    assert(speed <= 1000000 && order == LSBFIRST && mode == SPI_MODE0);
  }
};
class SPIClass {
  int operation = -1;
  size_t position{};
  std::vector<uint8_t> write_;

public:
  std::deque<std::vector<uint8_t>> replies;
  std::function<void(const std::vector<uint8_t> &)> command;
  void beginTransaction(const SPISettings &) {
    operation = -1;
    position = 0;
    write_.clear();
  }
  uint8_t transfer(uint8_t value) {
    if (operation == -1) {
      operation = value;
      return 0;
    }
    if (operation == 1) {
      write_.push_back(value);
      return 0;
    }
    if (operation == 2)
      return replies.empty() ? 0 : 1;
    if (operation == 3 && !replies.empty() && position < replies.front().size())
      return replies.front()[position++];
    return 0;
  }
  void endTransaction() {
    if (operation == 3 && !replies.empty() &&
        position >= replies.front().size())
      replies.pop_front();
    if (operation == 1) {
      if (write_ == std::vector<uint8_t>({0, 0, 255, 0, 255, 0})) {
        replies.clear();
        return;
      }
      assert(write_.size() >= 9);
      assert(write_[0] == 0 && write_[1] == 0 && write_[2] == 255);
      assert(uint8_t(write_[3] + write_[4]) == 0);
      assert(write_.size() == size_t(write_[3] + 7));
      uint8_t sum = 0;
      for (size_t i = 5; i + 1 < write_.size(); ++i)
        sum += write_[i];
      assert(sum == 0);
      command(write_);
    }
  }
};
