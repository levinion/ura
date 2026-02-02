#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include "ura/core/server.hpp"

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

private:
  void setup();
};

} // namespace ura
