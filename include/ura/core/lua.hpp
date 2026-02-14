#pragma once

#include <expected>
#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>

namespace ura {
class Lua {
public:
  sol::state state;
  sol::table ura;
  std::string lua_stdout;

  void init();
  std::expected<std::string, std::string> execute(std::string_view script);
  std::expected<std::string, std::string> execute_file(std::string_view path);
  std::optional<std::string> find_config_path();
  std::expected<void, std::string> load_config();
};

} // namespace ura
