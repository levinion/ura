#pragma once

#include <cassert>
#include <nlohmann/detail/value_t.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <sol/forward.hpp>
#include <sol/object.hpp>
#include <sol/state.hpp>
#include <sol/types.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"
#include "ura/util/util.hpp"
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

namespace flexible {

using object = sol::object;
using table = sol::table;
using function = sol::protected_function;
using nil = sol::nil_t;
using json = nlohmann::json;

static sol::table create_table() {
  return ura::UraServer::get_instance()->lua->state.create_table();
}

template<typename T>
void set(table t, std::string_view key, T&& value) {
  auto keys = ura::split(key, '.');
  auto current_table = t;
  for (size_t i = 0; i < keys.size() - 1; ++i) {
    auto& k = keys[i];
    current_table = current_table[k].get_or_create<sol::table>();
  }
  current_table[keys.back()] = value;
}

template<typename T>
std::optional<T> get(table t, std::string_view key) {
  auto keys = ura::split(key, '.');
  auto current_table = t;
  for (size_t i = 0; i < keys.size() - 1; ++i) {
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

static json to_json(object obj) {
  if (obj.is<sol::nil_t>())
    return {};
  if (obj.is<int>())
    return obj.as<int>();
  if (obj.is<uint64_t>())
    return obj.as<uint64_t>();
  if (obj.is<double>())
    return obj.as<double>();
  if (obj.is<std::string>())
    return obj.as<std::string>();
  if (obj.is<sol::table>()) {
    auto src = obj.as<table>();
    bool is_array = true;
    for (auto& [key, _] : src) {
      if (!key.is<int>())
        is_array = false;
    }
    if (!is_array) {
      auto dst = nlohmann::json::object();
      for (auto& [key, v] : src) {
        if (key.is<std::string>())
          dst[key.as<std::string>()] = to_json(v);
        if (key.is<int>()) {
          dst[std::to_string(key.as<int>())] = to_json(v);
        }
      }
      return dst;
    } else {
      auto dst = nlohmann::json::array();
      for (auto& [_, v] : src) {
        dst.push_back(to_json(v));
      }
      return dst;
    }
  }
  return {};
}

static flexible::object from(json j) {
  auto state = ura::UraServer::get_instance()->lua->state.lua_state();
  if (j.is_null())
    return flexible::nil {};
  if (j.is_boolean())
    return sol::make_object(state, j.get<bool>());
  if (j.is_number_integer())
    return sol::make_object(state, j.get<int>());
  if (j.is_number_unsigned())
    return sol::make_object(state, j.get<uint64_t>());
  if (j.is_number_float())
    return sol::make_object(state, j.get<double>());
  if (j.is_string()) {
    return sol::make_object(state, j.get<std::string>());
  }
  if (j.is_array()) {
    auto src = j.array();
    auto dst = create_table();
    for (auto& v : src) {
      dst.add(from(v));
    }
    return dst;
  }
  if (j.is_object()) {
    auto src = j.object();
    auto dst = create_table();
    for (auto& [k, v] : src.items()) {
      dst[k] = from(v);
    }
    return dst;
  }
  return flexible::nil {};
}

static object from_str(std::string_view str) {
  auto result = nlohmann::json::parse(str);
  return from(result);
}

} // namespace flexible
