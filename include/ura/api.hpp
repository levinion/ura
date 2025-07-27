#pragma once

#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura::api {

void set_keymap(
  std::string modifiers,
  std::string key,
  sol::protected_function f
);
void terminate();
void close_window();
void set_window_fullscreen(bool flag);
void reload();
void set_keyboard_repeat(int rate, int delay);
void set_env(std::string name, std::string value);
int switch_workspace(int index);
int move_window_to_workspace(int index);
int get_current_workspace_index();
void set_hook(std::string, sol::protected_function f);
void set_cursor_theme(std::string theme, int size);
void set_cursor_visible(bool flag);
void cursor_absolute_move(double x, double y);
void cursor_relative_move(double delta_x, double delta_y);
void set_cursor_shape(std::string name);
int get_current_window_index();
bool focus_window(int index);
void set_window_floating(bool flag);
bool is_window_fullscreen();
bool is_window_floating();
void lua_print(sol::variadic_args va);
} // namespace ura::api
