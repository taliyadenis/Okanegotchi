#include "nfc/ndef_parser.hpp"

#include <cctype>

namespace okanegachi::nfc {
namespace {

constexpr char kPrefix[] = "okanegachi:demo:v1:";
constexpr std::size_t kPrefixLen = sizeof(kPrefix) - 1;

bool language_ok(const std::uint8_t *lang, std::size_t len) {
  for (std::size_t i = 0; i < len; ++i) {
    const auto b = lang[i];
    if (!((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || b == '-'))
      return false;
  }
  return len >= 1 && len <= 8;
}

ParseResult action_from_text(const std::string &text) {
  if (text.rfind("okanegachi:", 0) == 0) {
    if (text.size() < kPrefixLen)
      return ParseResult::Invalid;
    if (text.compare(0, kPrefixLen, kPrefix) != 0)
      return ParseResult::Invalid;
    const std::string action = text.substr(kPrefixLen);
    if (action == "food")
      return ParseResult::Food;
    if (action == "ride")
      return ParseResult::Ride;
    if (action == "savings")
      return ParseResult::Savings;
    if (action == "review")
      return ParseResult::Review;
    return ParseResult::Invalid;
  }
  return ParseResult::Unrelated;
}

} // namespace

std::string parse_result_name(ParseResult r) {
  switch (r) {
  case ParseResult::Food:
    return "food";
  case ParseResult::Ride:
    return "ride";
  case ParseResult::Savings:
    return "savings";
  case ParseResult::Review:
    return "review";
  case ParseResult::Unrelated:
    return "unrelated";
  case ParseResult::Invalid:
    return "invalid";
  }
  return "invalid";
}

ParseResult parse_type2_user_memory(const std::uint8_t *data, std::size_t len) {
  if (!data || len == 0 || len > 144)
    return ParseResult::Invalid;
  std::size_t i = 0;
  const std::uint8_t *message = nullptr;
  std::size_t message_len = 0;
  bool terminated = false;
  while (i < len) {
    const std::uint8_t typ = data[i++];
    if (typ == 0)
      continue;
    if (typ == 0xFE) {
      terminated = true;
      break;
    }
    if (i >= len)
      return ParseResult::Invalid;
    const std::uint8_t size = data[i++];
    if (size == 0xFF || i + size > len)
      return ParseResult::Invalid;
    const std::uint8_t *value = data + i;
    i += size;
    if (typ == 1 || typ == 2) {
      if (size != 3)
        return ParseResult::Invalid;
    } else if (typ == 3) {
      if (message)
        return ParseResult::Invalid;
      message = value;
      message_len = size;
    } else {
      return ParseResult::Invalid;
    }
  }
  if (!message || !terminated)
    return ParseResult::Invalid;
  if (message_len < 7 || message[0] != 0xD1 || message[1] != 1 ||
      message[3] != 0x54)
    return ParseResult::Invalid;
  if (message_len != static_cast<std::size_t>(4 + message[2]))
    return ParseResult::Invalid;
  const std::uint8_t *payload = message + 4;
  const std::size_t payload_len = message_len - 4;
  const std::uint8_t status = payload[0];
  const std::size_t langlen = status & 0x3F;
  if (status & 0xC0)
    return ParseResult::Invalid;
  if (langlen < 1 || langlen > 8 || 1 + langlen >= payload_len)
    return ParseResult::Invalid;
  if (!language_ok(payload + 1, langlen))
    return ParseResult::Invalid;
  std::string text(reinterpret_cast<const char *>(payload + 1 + langlen),
                   payload_len - 1 - langlen);
  for (char c : text) {
    if (c == '\0')
      return ParseResult::Invalid;
  }
  return action_from_text(text);
}

ParseResult parse_type2_user_memory(const std::vector<std::uint8_t> &data) {
  return parse_type2_user_memory(data.data(), data.size());
}

} // namespace okanegachi::nfc
