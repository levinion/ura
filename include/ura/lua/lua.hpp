#pragma once

#include <any>
#include <expected>
#include <functional>
#include <memory>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <filesystem>
#include <unordered_map>
#include "ura/core/server.hpp"
#include "ura/util/util.hpp"

namespace ura {
class Lua {
public:
  sol::state state;
  sol::table ura;
  std::string lua_stdout;
  bool reset = false;
  std::unordered_map<std::string, sol::protected_function> hooks;
  std::unordered_map<
    std::string,
    std::unordered_map<uint64_t, sol::protected_function>>
    keymaps; // mode -> id -> func
  std::string mode = "normal";
  std::unordered_map<std::string, sol::protected_function> layouts;

  static std::unique_ptr<Lua> init();
  std::expected<std::string, std::string> execute(std::string script);
  std::expected<std::string, std::string> execute_file(std::filesystem::path);
  bool try_execute_keybinding(uint64_t id);
  bool contains_keybinding(uint64_t id);
  std::optional<std::filesystem::path> find_init_path();
  void try_execute_init();

  template<typename... Args>
  std::optional<sol::object>
  try_execute_hook(std::string name, Args&&... args) {
    if (!this->hooks.contains(name))
      return {};
    auto result = this->hooks[name](std::forward<Args>(args)...);
    if (!result.valid())
      return {};
    return result.template get<sol::object>();
  }

  template<typename T>
  void set(sol::table table, std::string_view key, T value) {
    auto keys = split(key, '.');
    auto current_table = table;
    for (size_t i = 0; i < keys.size() - 1; ++i) {
      auto& k = keys[i];
      current_table = current_table[k].get_or_create<sol::table>();
    }
    current_table[keys.back()] = value;
  }

  template<typename T>
  void set(std::string key, T value) {
    this->set<T>(this->ura, key, value);
  }

  template<typename T>
  std::optional<T> fetch(sol::table table, std::string_view key) {
    auto keys = split(key, '.');
    auto current_table = table;
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

  template<typename T>
  std::optional<T> fetch(const std::string& key) {
    if (this->cache.contains(key))
      return std::any_cast<T>(this->cache[key]);
    auto result = this->fetch<T>(this->ura, key);
    if (result)
      this->cache[key] = result.value();
    return result;
  }

  void setup();

private:
  template<typename T>
  sol::protected_function create_protected_function(T f) {
    auto table = this->state.create_table();
    table["f"] = f;
    return table["f"];
  }

  std::unordered_map<std::string, std::any> cache;
};

} // namespace ura
