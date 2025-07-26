#pragma once

#include <memory>
#include <sol/sol.hpp>
#include <filesystem>
#include "ura/util.hpp"

namespace ura {
class Lua {
public:
  static std::unique_ptr<Lua> init();
  void execute(std::string script);
  void execute_file(std::filesystem::path);
  bool try_execute_hook(std::string name);
  bool try_execute_keybinding(uint64_t id);
  std::optional<std::filesystem::path> find_init_path();
  bool try_execute_init();

  template<typename T>
  void set(std::string key, T value) {
    auto keys = split(key, '.');
    auto current_table = this->ura;
    for (size_t i = 0; i < keys.size() - 1; ++i) {
      auto& k = keys[i];
      current_table = current_table[k].get_or_create<sol::table>();
    }
    current_table[keys.back()] = value;
  }

  template<typename T>
  std::optional<T> fetch(std::string key) {
    auto keys = split(key, '.');
    auto current_table = this->ura;
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

private:
  sol::state state;

  sol::table ura;
  void setup();
};
} // namespace ura
