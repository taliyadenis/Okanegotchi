#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace okanegachi::core {

enum class ClipMode { Loop, Once };

struct ClipTiming {
  std::vector<int> durations_ms;
  ClipMode mode{ClipMode::Loop};
};

int sample_frame(const ClipTiming& clip, int elapsed_ms);
int sample_frame(const std::vector<int>& durations_ms, const std::string& mode, int elapsed_ms);

} // namespace okanegachi::core
