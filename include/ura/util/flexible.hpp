#pragma once

#include <cassert>
#include <nlohmann/detail/value_t.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <sol/forward.hpp>
#include <sol/object.hpp>
#include <sol/state.hpp>
#include <sol/types.hpp>
#include <string_view>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <absl/strings/str_split.h>

namespace flexible {

using object = sol::object;
using table = sol::table;
using function = sol::protected_function;
using json = nlohmann::json;

sol::nil_t nil();
sol::table create_table();

template<typename T>
void set(table& t, std::string_view key, T&& value) {
  std::vector<std::string> keys = absl::StrSplit(key, '.');
  auto current_table = t;
  for (std::size_t i = 0; i < keys.size() - 1; ++i) {
    auto& k = keys[i];
    current_table = current_table[k].get_or_create<sol::table>();
  }
  current_table[keys.back()] = value;
}

template<typename T>
std::optional<T> get(table& t, std::string_view key) {
  std::vector<std::string> keys = absl::StrSplit(key, '.');
  auto current_table = t;
  for (std::size_t i = 0; i < keys.size() - 1; ++i) {
    auto& k = keys[i];
    auto next_table = current_table.get<std::optional<sol::table>>(k);
    if (!next_table)
      return {};
    current_table = next_table.value();
  }
  if (!current_table.is<sol::table>())
    return {};
  return current_table.get<std::optional<T>>(keys.back());
}

json to_json(object& obj);
flexible::object from(json& j);
object from_str(std::string_view str);
} // namespace flexible
