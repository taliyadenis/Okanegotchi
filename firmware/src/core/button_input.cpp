#include "core/button_input.hpp"

namespace okanegachi::core {

char ButtonInput::name(ButtonId id) {
  switch (id) {
  case ButtonId::A:
    return 'A';
  case ButtonId::B:
    return 'B';
  case ButtonId::C:
    return 'C';
  }
  return '?';
}

void ButtonInput::set_held_on_boot(ButtonId id, bool held) {
  auto &s = buttons_[static_cast<std::size_t>(id)];
  s.raw = held;
  s.observed = held;
  s.stable = held;
  s.ignore_until_release = held;
}

void ButtonInput::set_raw(ButtonId id, bool down) {
  buttons_[static_cast<std::size_t>(id)].raw = down;
}

void ButtonInput::sync_time(std::uint64_t now_ms) {
  for (auto &s : buttons_)
    s.last_change_ms = now_ms;
}

void ButtonInput::emit_short(ButtonId id) {
  if (trace_)
    trace_(ButtonTrace{"button_short", std::string(1, name(id))});
}

void ButtonInput::emit_long(ButtonId id) {
  if (trace_)
    trace_(ButtonTrace{"button_long", std::string(1, name(id))});
}

void ButtonInput::advance(std::uint64_t now_ms) {
  for (std::size_t i = 0; i < buttons_.size(); ++i) {
    auto &s = buttons_[i];
    const ButtonId id = static_cast<ButtonId>(i);
    if (s.raw != s.observed) {
      s.observed = s.raw;
      s.last_change_ms = now_ms;
    }
    if (s.raw != s.stable) {
      const std::uint64_t elapsed = now_ms - s.last_change_ms;
      if (elapsed >= kDebounceMs) {
        s.stable = s.raw;
        s.last_change_ms = now_ms;
        if (s.stable) {
          s.down_since_ms = now_ms;
          s.long_fired = false;
        } else {
          if (!s.ignore_until_release && !s.long_fired)
            emit_short(id);
          s.ignore_until_release = false;
        }
      }
    } else if (s.stable && !s.ignore_until_release && !s.long_fired) {
      const std::uint64_t held = now_ms - s.down_since_ms;
      if (held >= kLongMs) {
        s.long_fired = true;
        emit_long(id);
      }
    }
  }
}

} // namespace okanegachi::core
