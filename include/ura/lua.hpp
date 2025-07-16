#pragma once

#include <memory>
#include <sol/sol.hpp>
#include <filesystem>

namespace ura {
class Lua {
public:
  sol::state state;
  sol::table table;

  static std::unique_ptr<Lua> init();
  void execute(std::string script);
  void execute_file(std::filesystem::path);

private:
  void register_function();
};
} // namespace ura
