#pragma once

#include <cstdint>
#include <sol/sol.hpp>
#include <string>
#include <vector>
#include "ura/util/flexible.hpp"

namespace ura::api::core {
// api
void terminate();
void spawn(std::string cmd);
void notify(std::string summary, std::string body);
std::optional<int>
set_timer(flexible::function f, int64_t value, int64_t interval);
void clear_timer(int fd);
// idle
void notify_idle_activity();
void set_idle_inhibitor(bool flag);
// win
void close_window(uint64_t id);
std::optional<uint64_t> get_current_window();
std::optional<uint64_t> get_window_output(uint64_t id);
void focus_window(uint64_t id);
void set_window_z_index(uint64_t id, int z);
void set_window_draggable(uint64_t id, bool flag);
std::optional<int> get_window_z_index(uint64_t id);
std::optional<bool> is_window_draggable(uint64_t id);
void activate_window(uint64_t id);
void move_window(uint64_t id, int x, int y);
void resize_window(uint64_t id, int width, int height);
std::optional<std::string> get_window_app_id(uint64_t id);
std::optional<std::string> get_window_title(uint64_t id);
void set_window_fullscreen(uint64_t id, bool flag);
std::optional<bool> is_window_fullscreen(uint64_t id);
flexible::object get_window_geometry(uint64_t id);
void set_window_tags(uint64_t id, std::vector<std::string> tags);
flexible::object get_window_tags(uint64_t id);
flexible::object get_all_windows();
std::optional<bool> is_window_mapped(uint64_t id);
std::optional<bool> is_window_focused(uint64_t id);

// input
void set_keyboard_repeat(int rate, int delay);
void set_cursor_theme(std::string theme, int size);
std::string get_cursor_theme();
int get_cursor_size();
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void set_cursor_shape(std::string name);
std::string get_cursor_shape();

// output
std::optional<uint64_t> get_current_output();
std::optional<uint64_t> get_output(std::string name);
flexible::object get_all_outputs();
std::optional<std::string> get_output_name(uint64_t id);
void set_output_dpms(uint64_t id, bool flag);
flexible::object get_output_logical_geometry(uint64_t id);
flexible::object get_output_usable_geometry(uint64_t id);
std::optional<float> get_output_scale(uint64_t id);
void set_output_tags(uint64_t id, std::vector<std::string> tags);
flexible::object get_output_tags(uint64_t id);

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
// userdata
void set_userdata(uint64_t id, flexible::object obj);
flexible::object get_userdata(uint64_t id);

// util
std::optional<uint64_t> get_keybinding_id(std::string pattern);
} // namespace ura::api::core

namespace ura::api::lua {
void print(sol::variadic_args va);
} // namespace ura::api::lua
