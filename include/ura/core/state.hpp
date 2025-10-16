#pragma once

#include <memory>
#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <unordered_map>
#include "flexible/flexible.hpp"

namespace ura {

class UraState {
public:
  std::unordered_map<std::string, flexible::function> hooks;
  std::
    unordered_map<std::string, std::unordered_map<uint64_t, flexible::function>>
      keymaps; // mode -> id -> func
  std::string keymap_mode = "normal";
  std::optional<std::string> config_path;

  static std::unique_ptr<UraState> init();

  template<typename T>
  std::optional<T> try_execute_hook(std::string name, flexible::object args) {
    if (!this->hooks.contains(name))
      return {};
    auto ret = this->hooks[name](args);
    return ret.as<T>();
  }

  void try_execute_hook(std::string name, flexible::object args);
  bool try_execute_keybinding(uint64_t id);
  bool contains_keybinding(uint64_t id);
};

} // namespace ura
