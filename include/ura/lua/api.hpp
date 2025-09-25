#pragma once

#include <optional>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura::api {

// api
void terminate();
void reload();
void spawn(std::string cmd);
void notify(std::string summary, std::string body);
void schedule(sol::protected_function f, int64_t time);
void set_hook(std::string name, sol::protected_function f);
void unset_hook(std::string name);
void notify_idle_activity();
void set_idle_inhibitor(bool flag);
// keymap
void keymap_set(std::string pattern, sol::protected_function f);
void keymap_set_mode(
  std::string mode,
  std::string pattern,
  sol::protected_function f
);
void keymap_unset(std::string pattern);
void keymap_unset_mode(std::string mode, std::string pattern);
void keymap_enter_mode(std::string mode);
std::string keymap_get_current_mode();
// win
void close_window(int index);
int get_window_number();
void move_window_to_workspace(int window_index, sol::object workspace_id);
void move_window_to_workspace_or_create(int window_index, int workspace_index);
std::optional<sol::table> get_current_window();
std::optional<sol::table> get_window(int index);
void focus_window(int index);
void set_window_layout(int index, std::string layout);
void set_window_z_index(int index, int z);
void set_window_draggable(int index, bool flag);
void activate_window(sol::object workspace_id, int window_index);
void move_window(int index, int x, int y);
void resize_window(int index, int width, int height);
void center_window(int index);
void swap_window(int index, int target);
void redraw_window(int index);
void set_window_pinned(int index, bool flag);
// input
void set_keyboard_repeat(int rate, int delay);
void set_pointer_properties(std::string pattern, sol::object object);
void set_cursor_theme(sol::object table);
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void set_cursor_shape(std::string name);
// ws
void create_workspace();
void switch_workspace(int index);
void switch_or_create_workspace(int index);
void destroy_workspace(int index);
int get_workspace_number();
std::optional<sol::table> get_current_workspace();
std::optional<sol::table> get_workspace(sol::object id);
sol::table list_workspaces();
void redraw_current_workspace();
// output
std::optional<sol::table> get_current_output();
std::optional<sol::table> get_output(std::string name);
void set_output_dpms(std::string name, bool flag);
void set_output_mode(std::string name, sol::object mode);
// layout
void set_layout(std::string name, sol::protected_function f);
void unset_layout(std::string name);
// fn
void set_env(std::string name, std::string value);
void unset_env(std::string name);
void append_lua_package_path(std::string path);
void prepend_lua_package_path(std::string path);
std::string expanduser(std::string path);
std::string expandvars(std::string path);
std::string expand(std::string path);
// override
void lua_print(sol::variadic_args va);
} // namespace ura::api
