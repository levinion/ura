#include "ura/util/json.hpp"
#include <glaze/json/generic.hpp>

namespace ura::util {

namespace {
struct strict_json_opts : glz::opts {
  bool validate_trailing_whitespace = true;
};
} // namespace

bool parse_json(std::string_view input, json& value) {
  return !glz::read<strict_json_opts{}>(value, input);
}

std::string stringify_json(const json& value) {
  std::string result;
  if (glz::write_json(value, result))
    return {};
  return result;
}

} // namespace ura::util
