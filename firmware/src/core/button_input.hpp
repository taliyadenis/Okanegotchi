#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace okanegachi::core {

enum class ButtonId { A, B, C };

struct ButtonTrace {
  std::string type;
  std::string button;
};

class ButtonInput {
public:
  using TraceFn = std::function<void(const ButtonTrace&)>;

  void set_trace(TraceFn fn) { trace_ = std::move(fn); }
  void set_held_on_boot(ButtonId id, bool held);
  void set_raw(ButtonId id, bool down);
  void sync_time(std::uint64_t now_ms);
  void advance(std::uint64_t now_ms);

private:
  struct State {
    bool raw{false};
    bool stable{false};
    bool ignore_until_release{false};
    bool long_fired{false};
    std::uint64_t last_change_ms{0};
    std::uint64_t down_since_ms{0};
  };
  std::array<State, 3> buttons_{};
  TraceFn trace_;
  static constexpr std::uint32_t kDebounceMs = 25;
  static constexpr std::uint32_t kLongMs = 1000;
  void emit_short(ButtonId id);
  void emit_long(ButtonId id);
  static char name(ButtonId id);
};

} // namespace okanegachi::core
