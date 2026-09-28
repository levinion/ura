#include "ura/util/json.hpp"
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

using ura::util::json;
using ura::util::parse_json;
using ura::util::stringify_json;

namespace {

bool test_primitives() {
  json value;
  if (!parse_json(
        R"({"enabled":true,"nothing":null,"name":"ura","ratio":1.25})",
        value
      ))
    return false;

  const bool fields_match = value.is_object() && value["enabled"].is_boolean()
    && value["enabled"].get<bool>() && value["nothing"].is_null()
    && value["name"].get<std::string>() == "ura" && value["ratio"].is_double()
    && value["ratio"].get<double>() == 1.25;
  return fields_match && parse_json("true \n\t", value) && value.get<bool>();
}

bool test_integer_ranges() {
  json value;
  constexpr auto input =
    R"({"positive":18446744073709551615,"negative":-9223372036854775808})";
  if (!parse_json(input, value))
    return false;

  return value["positive"].is_uint64()
    && value["positive"].get<uint64_t>() == std::numeric_limits<uint64_t>::max()
    && value["negative"].is_int64()
    && value["negative"].get<int64_t>() == std::numeric_limits<int64_t>::min();
}

bool test_nested_round_trip() {
  constexpr auto input =
    R"({"items":[1,"two",{"ok":true}],"escaped":"line\nbreak"})";
  json parsed;
  if (!parse_json(input, parsed))
    return false;

  auto serialized = stringify_json(parsed);
  json reparsed;
  if (serialized.empty() || !parse_json(serialized, reparsed))
    return false;

  const auto& items = reparsed["items"].get_array();
  return items.size() == 3 && items[0].get<uint64_t>() == 1
    && items[1].get<std::string>() == "two" && items[2]["ok"].get<bool>()
    && reparsed["escaped"].get<std::string>() == "line\nbreak";
}

bool test_rejects_invalid_json() {
  json value;
  return !parse_json(R"({"ok":true} trailing)", value)
    && !parse_json(R"({"missing": })", value);
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test case name\n";
    return 2;
  }

  const std::string_view name { argv[1] };
  bool passed = false;
  if (name == "primitives")
    passed = test_primitives();
  else if (name == "integer-ranges")
    passed = test_integer_ranges();
  else if (name == "nested-round-trip")
    passed = test_nested_round_trip();
  else if (name == "invalid-json")
    passed = test_rejects_invalid_json();
  else {
    std::cerr << "Unknown test case: " << name << '\n';
    return 2;
  }

  if (!passed) {
    std::cerr << "Test failed: " << name << '\n';
    return 1;
  }
  return 0;
}
