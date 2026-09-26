#include "app/app_engine.hpp"

#include <ArduinoJson.h>
#include <iostream>
#include <string>

int main() {
  std::string line;
  if (!std::getline(std::cin, line)) return 1;
  JsonDocument input;
  if (deserializeJson(input, line)) return 1;
  okanegachi::app::AppEngine engine;
  std::string seed;
  serializeJson(input["seed"], seed);
  engine.init_from_seed(seed, "mvp/examples/sync-response.json");
  std::string steps;
  serializeJson(input["steps"], steps);
  engine.run_steps(steps);
  JsonDocument out;
  deserializeJson(out["snapshot"], engine.snapshot_json());
  deserializeJson(out["trace"], engine.trace_json());
  deserializeJson(out["requests"], engine.requests_json());
  std::string result;
  serializeJson(out, result);
  std::cout << result << '\n';
  return 0;
}
