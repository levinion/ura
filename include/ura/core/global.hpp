#pragma once

#include "ura/util/flexible.hpp"

enum class UraGlobalType { Toplevel, Output };

class UraGlobal {
public:
  UraGlobal() = default;
  UraGlobal(UraGlobalType&& t) : type(t) {};
  UraGlobalType type;
  flexible::object userdata;

  template<typename T>
  std::optional<T> get_userdata(std::string_view name) {
    if (!this->userdata.is<sol::table>())
      return {};
    return this->userdata.as<sol::table>().get<std::optional<T>>(name);
  }
};
