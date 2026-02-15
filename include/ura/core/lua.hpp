#pragma once

#include <expected>
#include <sol/forward.hpp>
#include <sol/sol.hpp>

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
};

} // namespace ura
