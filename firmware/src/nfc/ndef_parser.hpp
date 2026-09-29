#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace okanegachi::nfc {

enum class ParseResult { Food, Ride, Savings, Review, Unrelated, Invalid };

ParseResult parse_type2_user_memory(const std::uint8_t *data, std::size_t len);
ParseResult parse_type2_user_memory(const std::vector<std::uint8_t> &data);
std::string parse_result_name(ParseResult r);

} // namespace okanegachi::nfc
