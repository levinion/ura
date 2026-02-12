#pragma once

#include <memory>
#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <unordered_map>
#include "ura/util/flexible.hpp"

namespace ura {

class UraState {
public:
  std::unordered_map<std::string, flexible::function> hooks;
  std::
    unordered_map<std::string, std::unordered_map<uint64_t, flexible::function>>
      keymaps; // mode -> id -> func
  std::string keymap_mode = "normal";
  std::optional<std::string> config_path;
  std::unordered_map<std::string, flexible::object> options;

  static std::unique_ptr<UraState>
  init(std::optional<std::string>&& config_path);

  template<typename T>
  std::optional<T> emit_hook(std::string name, flexible::object args) {
    if (!this->hooks.contains(name))
      return {};
    auto ret = this->hooks[name](args);
    if (!ret.valid())
      return {};
    auto obj = ret.get<flexible::object>();
    if (obj.is<T>())
      return obj.as<T>();
    return {};
  }

  void emit_hook(std::string name, flexible::object args);
  bool emit_keybinding(uint64_t id);
  bool contains_keybinding(uint64_t id);
  void set_option(std::string_view key, flexible::object& value);

  template<typename T>
  std::optional<T> get_option(std::string_view key) {
    if (!this->options.contains(std::string(key))) {
      return {};
    }
    return this->options[std::string(key)].as<std::optional<T>>();
  }
};

} // namespace ura
