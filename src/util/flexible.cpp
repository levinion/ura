#include "ura/util/flexible.hpp"
#include <glaze/json/generic.hpp>
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
    return nullptr;
  if (obj.get_type() == sol::type::boolean)
    return obj.as<bool>();
  if (obj.get_type() == sol::type::number) {
    double val = obj.as<double>();
    double iptr;
    if (std::modf(val, &iptr) == 0.0
        && val >= static_cast<double>(std::numeric_limits<int64_t>::min())
        && val <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
      return static_cast<int64_t>(val);
    } else {
      return val;
    }
  }
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
      auto dst = json::object_t{};
      for (auto& [key, v] : src) {
        if (key.is<std::string>())
          dst[key.as<std::string>()] = to_json(v);
        if (key.is<int64_t>()) {
          dst[std::to_string(key.as<int64_t>())] = to_json(v);
        }
      }
      return dst;
    } else {
      auto dst = json::array_t{};
      for (auto& [_, v] : src) {
        dst.push_back(to_json(v));
      }
      return dst;
    }
  }
  return nullptr;
}

flexible::object from(json& j) {
  auto state = ura::UraServer::get_instance()->lua->state.lua_state();
  if (j.is_null())
    return sol::nil;
  if (j.is_boolean())
    return sol::make_object(state, j.get<bool>());
  if (j.is_int64())
    return sol::make_object(state, j.get<int64_t>());
  if (j.is_uint64())
    return sol::make_object(state, j.get<uint64_t>());
  if (j.is_double())
    return sol::make_object(state, j.get<double>());
  if (j.is_string()) {
    return sol::make_object(state, j.get<std::string>());
  }
  if (j.is_array()) {
    auto dst = create_table();
    int i = 1;
    for (auto& v : j.get_array()) {
      dst[i++] = from(v);
    }
    return dst;
  }
  if (j.is_object()) {
    auto dst = create_table();
    for (auto& [k, v] : j.get_object()) {
      dst[k] = from(v);
    }
    return dst;
  }
  return sol::nil;
}

object from_str(std::string_view str) {
  json result;
  if (!ura::util::parse_json(str, result))
    return {};
  return from(result);
}
} // namespace flexible
