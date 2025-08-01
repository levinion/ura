#pragma once

#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura::api {
// api
void set_keymap(
  std::string modifiers,
  std::string key,
  sol::protected_function f
);
void set_env(std::string name, std::string value);
void terminate();
void reload();
void set_hook(std::string, sol::protected_function f);
// win
void close_window();
void set_window_fullscreen(bool flag);
int get_window_number();
int move_window_to_workspace(int index);
int get_current_window_index();
bool focus_window(int index);
void set_window_floating(bool flag);
bool is_window_fullscreen();
bool is_window_floating();
void move_window_to_scratchpad();
// input
void set_keyboard_repeat(int rate, int delay);
void set_cursor_theme(std::string theme, int size);
void set_cursor_visible(bool flag);
void cursor_absolute_move(double x, double y);
void cursor_relative_move(double delta_x, double delta_y);
void set_cursor_shape(std::string name);
// ws
int switch_workspace(int index);
void destroy_workspace(int index);
int get_current_workspace_index();
int get_workspace_number();
// override
void lua_print(sol::variadic_args va);
} // namespace ura::api
