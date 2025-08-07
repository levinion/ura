#pragma once

#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura::api {

// api
void set_keymap(std::string pattern, sol::protected_function f);
void terminate();
void reload();
void set_hook(std::string, sol::protected_function f);
// win
void close_window(int index);
int get_window_number();
void move_window_to_workspace(int window_index, int workspace_index);
std::optional<sol::table> get_current_window();
std::optional<sol::table> get_window(int index);
bool focus_window(int index);
void set_window_floating(int index, bool flag);
void set_window_fullscreen(int index, bool flag);
void activate_window(int workspace_index, int window_index);
void move_window(int index, int x, int y);
void resize_window(int index, int width, int height);
void center_window(int index);
// input
void set_keyboard_repeat(int rate, int delay);
void set_cursor_theme(std::string theme, int size);
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void cursor_absolute_move(double x, double y);
void cursor_relative_move(double delta_x, double delta_y);
void set_cursor_shape(std::string name);
// ws
void switch_workspace(int index);
void destroy_workspace(int index);
int get_workspace_number();
sol::table get_current_workspace();
std::optional<sol::table> get_workspace(int index);
sol::table list_workspaces();
// output
sol::table get_current_output();
// fn
void set_env(std::string name, std::string value);
void unset_env(std::string name);
void append_lua_package_path(std::string path);
void prepend_lua_package_path(std::string path);
// override
void lua_print(sol::variadic_args va);
} // namespace ura::api
