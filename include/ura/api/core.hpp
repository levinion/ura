#pragma once

#include <cstdint>
#include <sol/sol.hpp>
#include <string>
#include "flexible/flexible.hpp"

namespace ura::api::core {
// api
void terminate();
void reload();
void spawn(std::string cmd);
void notify(std::string summary, std::string body);
void schedule(flexible::function f, int64_t time);
void eval(std::string code);
// hook
void set_hook(std::string name, flexible::function f);
void unset_hook(std::string name);
// idle
void notify_idle_activity();
void set_idle_inhibitor(bool flag);
// keymap
void set_keymap(std::string pattern, std::string mode, flexible::function f);
void unset_keymap(std::string pattern, std::string mode);
void set_keymap_mode(std::string mode);
std::string get_keymap_mode();
// win
void close_window(uint64_t id);
void move_window_to_workspace(uint64_t id, uint64_t workspace_id);
std::optional<uint64_t> get_current_window();
std::optional<uint64_t> get_window(uint64_t workspace_id, int index);
flexible::object get_windows(uint64_t workspace_id);
std::optional<uint64_t> get_window_output(uint64_t id);
std::optional<int> get_window_index(uint64_t id);
void focus_window(uint64_t id);
void set_window_z_index(uint64_t id, int z);
void set_window_draggable(uint64_t id, bool flag);
std::optional<int> get_window_z_index(uint64_t id);
std::optional<bool> is_window_draggable(uint64_t id);
void activate_window(uint64_t id);
void move_window(uint64_t id, int x, int y);
void resize_window(uint64_t id, int width, int height);
void swap_window(uint64_t id, uint64_t target);
std::optional<std::string> get_window_app_id(uint64_t id);
std::optional<std::string> get_window_title(uint64_t id);
void set_window_fullscreen(uint64_t id, bool flag);
std::optional<bool> is_window_fullscreen(uint64_t id);
flexible::object get_window_geometry(uint64_t id);
std::optional<uint64_t> get_window_workspace(uint64_t id);
// input
void set_keyboard_repeat(int rate, int delay);
void set_pointer_accel_profile(std::string profile);
void set_pointer_move_speed(double speed);
void set_pointer_scroll_speed(double speed);
void set_cursor_theme(std::string theme, int size);
std::string get_cursor_theme();
int get_cursor_size();
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void set_cursor_shape(std::string name);
std::string get_cursor_shape();
// ws
void create_indexed_workspace();
void create_named_workspace(std::string name);
void switch_workspace(uint64_t id);
void destroy_workspace(uint64_t id);
std::optional<uint64_t> get_current_workspace();
std::optional<uint64_t> get_indexed_workspace(uint64_t output_id, int index);
std::optional<uint64_t> get_named_workspace(std::string name);
std::optional<int> get_workspace_index(uint64_t id);
std::optional<std::string> get_workspace_name(uint64_t id);
std::optional<bool> is_workspace_named(uint64_t id);
flexible::object get_workspaces();
flexible::object get_indexed_workspaces(uint64_t output_id);
flexible::object get_named_workspaces();

// output
std::optional<uint64_t> get_current_output();
std::optional<uint64_t> get_output(std::string name);
void set_output_dpms(uint64_t id, bool flag);
flexible::object get_output_logical_geometry(uint64_t id);
flexible::object get_output_usable_geometry(uint64_t id);
std::optional<float> get_output_scale(uint64_t id);
// fn
void set_env(std::string name, std::string value);
void unset_env(std::string name);
void append_package_path(std::string path);
void prepend_package_path(std::string path);
std::string expanduser(std::string path);
std::string expandvars(std::string path);
std::string expand(std::string path);
std::string to_json(flexible::object obj);
flexible::object parse_json(std::string str);
// opt
void set_option(std::string key, flexible::object obj);
flexible::object get_option(std::string key);
void set_userdata(uint64_t id, flexible::object obj);
flexible::object get_userdata(uint64_t id);
} // namespace ura::api::core
