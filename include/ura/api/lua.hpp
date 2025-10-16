#pragma once
#include <sol/forward.hpp>
#include <sol/sol.hpp>

namespace ura::api::lua {
void print(sol::variadic_args va);
void schedule(sol::protected_function f, int64_t time);
void set_hook(std::string name, sol::protected_function f);
void set_keymap(
  std::string pattern,
  std::string mode,
  sol::protected_function f
);
sol::object get_output_logical_geometry(uint64_t id);
} // namespace ura::api::lua
