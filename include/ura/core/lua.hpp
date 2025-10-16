#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include "ura/core/server.hpp"
#include "ura/util/util.hpp"

namespace ura {
class Lua {
public:
  sol::state state;
  sol::table ura;
  std::string lua_stdout;
  bool reset = false;

  static std::unique_ptr<Lua> init();
  std::expected<std::string, std::string> execute(std::string_view script);
  std::expected<std::string, std::string> execute_file(std::string_view path);
  std::optional<std::string> find_config_path();
  std::expected<void, std::string> load_config();
  std::optional<sol::protected_function> load_as_function(std::string_view f);

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
    return this->fetch<T>(this->ura, key);
  }

private:
  void setup();

  template<typename T>
  sol::protected_function create_protected_function(T f) {
    auto table = this->state.create_table();
    table["f"] = f;
    return table["f"];
  }
};

} // namespace ura
