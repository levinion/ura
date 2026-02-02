#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"

namespace flexible {

sol::nil_t nil() {
  return sol::nil;
}

sol::table create_table() {
  return ura::UraServer::get_instance()->lua->state.create_table();
}

json to_json(object& obj) {
  if (obj.is<sol::nil_t>())
    return {};
  if (obj.is<bool>())
    return obj.as<bool>();
  if (obj.is<int>())
    return obj.as<int>();
  if (obj.is<uint64_t>())
    return obj.as<uint64_t>();
  if (obj.is<double>())
    return obj.as<double>();
  if (obj.is<std::string>())
    return obj.as<std::string>();
  if (obj.is<sol::function>())
    return "<lua function>";
  if (obj.is<sol::table>()) {
    auto src = obj.as<table>();
    bool is_array = true;
    for (auto& [key, _] : src) {
      if (!key.is<int>()) {
        is_array = false;
        break;
      }
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

flexible::object from(json& j) {
  auto state = ura::UraServer::get_instance()->lua->state.lua_state();
  if (j.is_null())
    return sol::nil;
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
    auto src = j;
    auto dst = create_table();
    int i = 1;
    for (auto& v : src) {
      dst[i++] = from(v);
    }
    return dst;
  }
  if (j.is_object()) {
    auto src = j;
    auto dst = create_table();
    for (auto& [k, v] : src.items()) {
      dst[k] = from(v);
    }
    return dst;
  }
  return sol::nil;
}

object from_str(std::string_view str) {
  auto result = nlohmann::json::parse(str, nullptr, false, false);
  if (result.is_discarded())
    return {};
  return from(result);
}
} // namespace flexible
