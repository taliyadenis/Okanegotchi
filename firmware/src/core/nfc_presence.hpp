#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace okanegachi::core {

enum class NfcObservation { Absent, Present, Error };

struct NfcTagRead {
  std::vector<std::uint8_t> user_memory;
};

class NfcPresence {
public:
  void set_present(bool present);
  void set_error();
  void set_tag_memory(const std::vector<std::uint8_t>& mem);
  void advance(std::uint32_t delta_ms);
  bool consume_presentation(NfcTagRead* out);

private:
  NfcObservation obs_{NfcObservation::Absent};
  std::vector<std::uint8_t> memory_;
  bool latched_{false};
  std::uint32_t absence_ms_{0};
  bool armed_{true};
  static constexpr std::uint32_t kRearmMs = 500;
};

} // namespace okanegachi::core
