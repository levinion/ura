#pragma once

#include <expected>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include "ura/util/flexible.hpp"

namespace ura {
class Lua {
public:
  sol::state state;
  sol::table ura;
  std::string lua_stdout;

  static std::unique_ptr<Lua> init();
  void load_runtime();
  std::expected<std::string, std::string> execute(std::string_view script);
  std::expected<std::string, std::string> execute_file(std::string_view path);

  template<typename T>
  std::optional<T> emit_hook(std::string name, flexible::object args) {
    auto f = this->ura["hook"]["_hooks"][name]
               .get<std::optional<sol::protected_function>>();
    if (f) {
      auto ret = f.value()();
      if (!ret.valid())
        return {};
      auto obj = ret.get<flexible::object>();
      if (obj.is<T>())
        return obj.as<T>();
      return {};
    }
    return {};
  }

  void emit_hook(std::string name, flexible::object args);
  bool emit_keybinding(uint64_t id);
  bool contains_keybinding(uint64_t id);
  void set_option(std::string_view key, flexible::object value);

  template<typename T>
  std::optional<T> get_option(std::string_view key) {
    return this->ura["opt"][key].get<std::optional<T>>();
  }
};

} // namespace ura
