#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include "ura/util/util.hpp"

namespace flexible {

class object;
using array = std::vector<object>;
using table = std::unordered_map<std::string, object>;

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
  void set(const std::string key, T&& value) {
    auto ft = this->as<table>();
    assert(ft);
    ft.value()[key] = value;
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

private:
  std::variant<int, double, bool, std::string, array, table> m;
};

} // namespace flexible
