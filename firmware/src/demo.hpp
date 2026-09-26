#pragma once
#include "engine.hpp"
#include <deque>
namespace okanegachi {
// A deliberately local fake server, used only by the LOCAL DEMO build and
// tests. Rebooting that build starts a new ledger/epoch. Never talks to a real
// backend.
class DemoServer {
public:
  DemoServer(Clock &, Random &);
  std::pair<int, std::string> sync(const Request &);
  bool scenario(const std::string &);
  bool choose_pet(const std::string &);

private:
  Clock &clock_;
  Random &random_;
  JsonDocument state_;
  std::string epoch_;
  int seq_{};
  std::deque<Command> commands_;
  std::deque<std::pair<std::string, EventResult>> receipts_;
  void reset();
  void command(const char *, int duration = 8000);
  void refresh_finance();
};
} // namespace okanegachi
