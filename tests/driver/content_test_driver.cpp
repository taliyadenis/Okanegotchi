#include "core/animation_player.hpp"
#include "nfc/ndef_parser.hpp"

#include <ArduinoJson.h>
#include <iostream>
#include <string>

int main() {
  std::string line;
  if (!std::getline(std::cin, line)) return 1;
  JsonDocument input;
  if (deserializeJson(input, line)) return 1;
  const char* op = input["op"];
  JsonDocument out;
  if (op && std::string(op) == "parse_ndef") {
    const std::string hex = input["hex"].as<std::string>();
    std::vector<std::uint8_t> data;
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
      data.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    out["result"] = okanegachi::nfc::parse_result_name(okanegachi::nfc::parse_type2_user_memory(data));
  } else if (op && std::string(op) == "sample_clip") {
    std::vector<int> durations;
    for (int v : input["durations_ms"].as<JsonArrayConst>()) durations.push_back(v);
    const std::string mode = input["mode"].as<std::string>();
    const int elapsed = input["elapsed_ms"].as<int>();
    out["frame"] = okanegachi::core::sample_frame(durations, mode, elapsed);
  } else {
    return 1;
  }
  std::string result;
  serializeJson(out, result);
  std::cout << result << '\n';
  return 0;
}
