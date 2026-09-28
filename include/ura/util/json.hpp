#pragma once

#include <string>
#include <string_view>
#include <glaze/json/generic_fwd.hpp>

namespace ura::util {

using json = glz::generic_u64;

bool parse_json(std::string_view input, json& value);
std::string stringify_json(const json& value);

} // namespace ura::util
