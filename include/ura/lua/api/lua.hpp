#pragma once
#include <sol/forward.hpp>
#include <sol/sol.hpp>

namespace ura::api::lua {
void print(sol::variadic_args va);
void set_pointer_properties(std::string pattern, sol::object object);
void schedule(sol::protected_function f, int64_t time);
void set_hook(std::string name, sol::protected_function f);
void set_keymap(
  std::string pattern,
  std::string mode,
  sol::protected_function f
);
void set_layout(std::string name, sol::protected_function f);
} // namespace ura::api::lua
