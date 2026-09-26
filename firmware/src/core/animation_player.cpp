#include "core/animation_player.hpp"

namespace okanegachi::core {
namespace {

int total_ms(const std::vector<int>& durations) {
  int sum = 0;
  for (int d : durations) sum += d;
  return sum;
}

} // namespace

int sample_frame(const ClipTiming& clip, int elapsed_ms) {
  const std::string mode = clip.mode == ClipMode::Loop ? "loop" : "once";
  return sample_frame(clip.durations_ms, mode, elapsed_ms);
}

int sample_frame(const std::vector<int>& durations_ms, const std::string& mode, int elapsed_ms) {
  if (durations_ms.empty()) return 0;
  const int total = total_ms(durations_ms);
  if (total <= 0) return 0;
  int t = elapsed_ms;
  if (mode == "loop") {
    t = elapsed_ms % total;
  } else if (elapsed_ms >= total) {
    return static_cast<int>(durations_ms.size()) - 1;
  }
  int acc = 0;
  for (std::size_t i = 0; i < durations_ms.size(); ++i) {
    acc += durations_ms[i];
    if (t < acc) return static_cast<int>(i);
  }
  return static_cast<int>(durations_ms.size()) - 1;
}

} // namespace okanegachi::core
