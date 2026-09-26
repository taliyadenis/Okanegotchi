#include "core/demo_tag_router.hpp"

namespace okanegachi::core {

RouteDecision route_ndef(nfc::ParseResult parsed, bool demo_mode, bool online, bool demo_fresh) {
  RouteDecision d;
  if (parsed == nfc::ParseResult::Invalid) return d;
  if (parsed == nfc::ParseResult::Unrelated) {
    d.action = RoutedAction::GenericReview;
    d.review_source = "card";
    return d;
  }
  if (parsed == nfc::ParseResult::Review) {
    d.action = RoutedAction::DemoReview;
    d.review_source = "sticker";
    return d;
  }
  if (!online && (parsed == nfc::ParseResult::Food || parsed == nfc::ParseResult::Ride ||
                  parsed == nfc::ParseResult::Savings)) {
    return d;
  }
  if (demo_mode && online && demo_fresh) {
    if (parsed == nfc::ParseResult::Food) {
      d.action = RoutedAction::DemoFood;
      d.scenario = "food";
    } else if (parsed == nfc::ParseResult::Ride) {
      d.action = RoutedAction::DemoRide;
      d.scenario = "ride";
    } else if (parsed == nfc::ParseResult::Savings) {
      d.action = RoutedAction::DemoSavings;
      d.scenario = "savings";
    }
    return d;
  }
  if (parsed == nfc::ParseResult::Food || parsed == nfc::ParseResult::Ride ||
      parsed == nfc::ParseResult::Savings) {
    d.action = RoutedAction::GenericReview;
    d.review_source = "card";
  }
  return d;
}

} // namespace okanegachi::core
