#include "core/nfc_presence.hpp"

namespace okanegachi::core {

void NfcPresence::set_present(bool present) {
  obs_ = present ? NfcObservation::Present : NfcObservation::Absent;
  if (present) {
    latched_ = true;
    absence_ms_ = 0;
  }
}

void NfcPresence::set_error() {
  obs_ = NfcObservation::Error;
  absence_ms_ = 0;
}

void NfcPresence::set_tag_memory(const std::vector<std::uint8_t>& mem) { memory_ = mem; }

void NfcPresence::advance(std::uint32_t delta_ms) {
  if (obs_ == NfcObservation::Absent) {
    absence_ms_ += delta_ms;
    if (absence_ms_ >= kRearmMs) armed_ = true;
  } else if (obs_ == NfcObservation::Error) {
    absence_ms_ = 0;
  } else {
    absence_ms_ = 0;
  }
}

bool NfcPresence::consume_presentation(NfcTagRead* out) {
  if (!armed_ || !latched_ || obs_ != NfcObservation::Present) return false;
  latched_ = false;
  armed_ = false;
  if (out) {
    out->user_memory = memory_;
  }
  return true;
}

} // namespace okanegachi::core
