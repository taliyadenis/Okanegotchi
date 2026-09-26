#pragma once
#include "nfc/ndef_parser.hpp"
#include <optional>
#include <string>

namespace okanegachi::core {

enum class RoutedAction { None, DemoFood, DemoRide, DemoSavings, DemoReview, GenericReview };

struct RouteDecision {
  RoutedAction action{RoutedAction::None};
  std::string scenario;
  std::string review_source;
};

RouteDecision route_ndef(nfc::ParseResult parsed, bool demo_mode, bool online, bool demo_fresh);

} // namespace okanegachi::core
