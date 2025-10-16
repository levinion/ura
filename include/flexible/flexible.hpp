#pragma once

#include <cassert>
#include <functional>
#include <nlohmann/detail/value_t.hpp>
#include <optional>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/types.hpp>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <unordered_map>
#include "ura/util/util.hpp"
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

namespace flexible {

class object;
using array = std::vector<object>;
using table = std::unordered_map<std::string, object>;
using function = std::function<object(object)>;

struct null {};

class object {
public:
  template<typename T>
  static object init(T&& m) {
    auto obj = object();
    obj.m = m;
    return std::move(obj);
  }

  template<typename T>
  bool is() {
    return std::holds_alternative<T>(this->m);
  }

  template<typename T>
  std::optional<T> as() {
    if (!this->is<T>())
      return {};
    return std::get<T>(this->m);
  }

  template<typename T>
  std::optional<T> get(const std::string key) {
    if (auto ft = this->as<table>()) {
      if (!ft->contains(key))
        return {};
      return (*ft)[key].as<T>();
    }
    return {};
  }

  template<typename T>
  std::optional<T> recursive_get(const std::string keys) {
    auto v = ura::split(keys, '.');
    auto current = this;
    for (int i = 0; i < v.size() - 1; i++) {
      if (auto ft = current->as<table>()) {
        auto key = std::string(v[i]);
        if (!ft->contains(key))
          return {};
        current = &(*ft)[key];
      }
      return current->get<T>(std::string(v[v.size() - 1]));
    }
    return {};
  }

  template<typename T>
  void set(T&& value) {
    this->m = value;
  }

  template<typename T>
  void set(const std::string key, T&& value) {
    auto ft = this->as<table>();
    assert(ft);
    ft.value()[key].m = value;
  }

  template<typename T>
  void recursive_set(const std::string keys, T&& value) {
    assert(this->is<table>());
    auto v = ura::split(keys, '.');
    auto current = this;
    for (int i = 0; i < v.size() - 1; i++) {
      if (auto ft = current->as<table>()) {
        auto key = std::string(v[i]);
        if (!ft->contains(key))
          (*ft)[key] = object::init(table());
        current = &(*ft)[key];
      }
      return current->set<T>(std::string(v[v.size() - 1]));
    }
  }

  inline std::string to_json() {
    return this->to_json_value().dump();
  }

  inline static object from(std::string_view str) {
    auto j = nlohmann::json::parse(str);
    return object::from_json_value(j);
  }

  inline sol::object to_sol(sol::state_view state) noexcept {
    return this->to_sol_value(state);
  }

  inline static object from(sol::object obj) {
    return object::from_sol_value(obj);
  }

private:
  std::variant<
    int,
    int64_t,
    uint64_t,
    double,
    bool,
    std::string,
    array,
    table,
    null>
    m = null {};

  inline nlohmann::json to_json_value() {
    if (this->is<null>())
      return {};
    if (this->is<int>())
      return this->as<int>().value();
    if (this->is<int64_t>())
      return this->as<int64_t>().value();
    if (this->is<uint64_t>())
      return this->as<uint64_t>().value();
    if (this->is<double>())
      return this->as<double>().value();
    if (this->is<std::string>())
      return this->as<std::string>().value();
    if (this->is<array>()) {
      auto src = this->as<array>().value();
      auto dst = nlohmann::json::array();
      for (auto& v : src) {
        dst.push_back(v.to_json_value());
      }
      return dst;
    }
    if (this->is<table>()) {
      auto src = this->as<table>().value();
      auto dst = nlohmann::json::object();
      for (auto& [key, v] : src) {
        dst[key] = v.to_json_value();
      }
      return dst;
    }
    std::unreachable();
  }

  inline static object from_json_value(nlohmann::json j) {
    if (j.is_null())
      return object::init(null {});
    if (j.is_boolean())
      return object::init(j.get<bool>());
    if (j.is_number_unsigned()) {
      return object::init(j.get<uint64_t>());
    }
    if (j.is_number_integer()) {
      return object::init(j.get<int64_t>());
    }
    if (j.is_number_float()) {
      return object::init(j.get<double>());
    }
    if (j.is_string()) {
      return object::init(j.get<std::string>());
    }
    if (j.is_array()) {
      auto src = j.array();
      array dst;
      dst.reserve(src.size());
      for (auto& v : src) {
        dst.push_back(object::from_json_value(v));
      }
      return object::init(dst);
    }
    if (j.is_object()) {
      auto src = j.object();
      table dst;
      for (auto& [k, v] : src.items()) {
        dst[k] = object::from_json_value(v);
      }
      return object::init(dst);
    }
    std::unreachable();
  }

  inline sol::object to_sol_value(sol::state_view state) {
    if (this->is<null>())
      return {};
    if (this->is<int>())
      return sol::make_object(state, this->as<int>().value());
    if (this->is<int64_t>())
      return sol::make_object(state, this->as<int64_t>().value());
    if (this->is<uint64_t>())
      return sol::make_object(state, this->as<uint64_t>().value());
    if (this->is<double>())
      return sol::make_object(state, this->as<double>().value());
    if (this->is<std::string>())
      return sol::make_object(state, this->as<std::string>().value());
    if (this->is<array>()) {
      auto src = this->as<array>().value();
      auto dst = state.create_table();
      for (auto& v : src) {
        dst.set(v.to_sol_value(state));
      }
      return dst;
    }
    if (this->is<table>()) {
      auto src = this->as<table>().value();
      auto dst = state.create_table();
      for (auto& [key, v] : src) {
        dst.set(key, v.to_sol_value(state));
      }
      return dst;
    }
    std::unreachable();
  }

  inline static object from_sol_value(sol::object obj) {
    if (obj.is<sol::nil_t>())
      return {};
    if (obj.is<bool>())
      return object::init(obj.as<bool>());
    if (obj.is<int>()) {
      return object::init(obj.as<int>());
    }
    if (obj.is<uint64_t>()) {
      return object::init(obj.as<uint64_t>());
    }
    if (obj.is<int64_t>()) {
      return object::init(obj.as<int64_t>());
    }
    if (obj.is<double>()) {
      return object::init(obj.as<double>());
    }
    if (obj.is<std::string>()) {
      return object::init(obj.as<std::string>());
    }
    if (obj.is<std::vector<sol::object>>()) {
      auto src = obj.as<std::vector<sol::object>>();
      array dst;
      dst.reserve(src.size());
      for (auto& v : src) {
        dst.push_back(object::from_sol_value(v));
      }
      return object::init(dst);
    }
    if (obj.is<sol::table>()) {
      auto src = obj.as<sol::table>();
      table dst;
      for (auto& [k, v] : src.pairs()) {
        dst[k.as<std::string>()] = object::from_sol_value(v);
      }
      return object::init(dst);
    }
    std::unreachable();
  }
};
} // namespace flexible
