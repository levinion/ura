#pragma once

#include <cstdint>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>
#include <vector>
#include "ura/util/flexible.hpp"

namespace ura::api::core {
// api
void terminate();
void spawn(std::string cmd);
void notify(std::string summary, std::string body);
sol::optional<int> set_timeout(flexible::function f, double timeout);
void clear_timeout(int fd);
sol::optional<int> set_interval(flexible::function f, double timeout);
void clear_interval(int fd);
// idle
void notify_idle_activity();
void set_idle_inhibitor(bool flag);
// win
void close_window(uint64_t id);
sol::optional<uint64_t> get_current_window();
sol::optional<uint64_t> get_window_output(uint64_t id);
void focus_window(uint64_t id);
void set_window_z_index(uint64_t id, int z);
sol::optional<int> get_window_z_index(uint64_t id);
void activate_window(uint64_t id);
void move_window(uint64_t id, int x, int y);
void animate_window_move(
  uint64_t id,
  int x,
  int y,
  double duration,
  flexible::object easing
);
void resize_window(uint64_t id, int width, int height);
void animate_window_rotation(
  uint64_t id,
  double degrees,
  double duration,
  flexible::object easing
);
sol::optional<double> get_window_rotation(uint64_t id);
void animate_window_opacity(
  uint64_t id,
  float opacity,
  double duration,
  flexible::object easing
);
void animate_window_open(
  uint64_t id,
  double duration,
  flexible::object easing,
  flexible::object options
);
void animate_window_close(
  uint64_t id,
  double duration,
  flexible::object easing,
  flexible::object options
);
void animate_window_resize(
  uint64_t id,
  int width,
  int height,
  double duration,
  flexible::object easing
);
sol::optional<std::string> get_window_app_id(uint64_t id);
sol::optional<std::string> get_window_title(uint64_t id);
sol::optional<std::string> get_window_icon_name(uint64_t id);
flexible::object get_window_icon(uint64_t id);
void set_window_fullscreen(uint64_t id, bool flag);
sol::optional<bool> is_window_fullscreen(uint64_t id);
sol::optional<bool> is_window_fullscreen_requested(uint64_t id);
void set_window_resizing(uint64_t id, bool flag);
sol::optional<bool> is_window_resizing(uint64_t id);
void set_window_maximized(uint64_t id, bool flag);
sol::optional<bool> is_window_maximized(uint64_t id);
sol::optional<bool> is_window_maximized_requested(uint64_t id);
flexible::object get_window_geometry(uint64_t id);
flexible::object get_window_visual_geometry(uint64_t id);
void set_window_tags(uint64_t id, std::vector<std::string> tags);
flexible::object get_window_tags(uint64_t id);
flexible::object get_all_windows();
sol::optional<uint64_t> get_window_lru(uint64_t id);
sol::optional<bool> is_window_mapped(uint64_t id);
sol::optional<bool> is_window_focused(uint64_t id);
sol::optional<float> get_window_opacity(uint64_t id);
void set_window_opacity(uint64_t id, float opacity);
flexible::object define_effect(flexible::object spec);
bool set_window_effect_instance(
  uint64_t id,
  uint64_t effect_id,
  uint32_t generation,
  flexible::object uniforms,
  flexible::object options
);
flexible::object find_effect(std::string name);
flexible::object define_texture(std::string path);
flexible::object find_texture(std::string name);
bool undefine_texture(uint64_t id, uint32_t generation);
bool undefine_effect(uint64_t id, uint32_t generation);
void clear_effects();
void migrate_effects();
void animate_window_effect_uniform(
  uint64_t id,
  std::string name,
  flexible::object from,
  flexible::object to,
  double duration,
  flexible::object easing
);
void clear_window_effect(uint64_t id);
void set_window_border_color(uint64_t id, std::string color);
sol::optional<std::string> get_window_border_color(uint64_t id);

// input
void set_keyboard_repeat(int rate, int delay);
void set_cursor_theme(std::string theme, int size);
std::string get_cursor_theme();
int get_cursor_size();
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void set_cursor_shape(std::string name);
std::string get_cursor_shape();
sol::object get_cursor_pos();

// output
sol::optional<uint64_t> get_current_output();
sol::optional<uint64_t> get_output(std::string name);
flexible::object get_all_outputs();
sol::optional<std::string> get_output_name(uint64_t id);
flexible::object get_output_logical_geometry(uint64_t id);
flexible::object get_output_usable_geometry(uint64_t id);
sol::optional<float> get_output_scale(uint64_t id);
void set_output_tags(uint64_t id, std::vector<std::string> tags);
flexible::object get_tag(std::string name);
flexible::object get_tags();
flexible::object get_namespaces();
flexible::object get_active_tag(std::string name_space);
flexible::object get_current_tag();
bool set_current_tag(std::string name);
bool shift_tag_index(int delta);
bool set_tag_index(int index);
bool shift_window_tag_index(int delta, bool update_output);
bool switch_tag_namespace(int delta);
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
sol::optional<uint64_t> get_keybinding_id(std::string pattern);
long time_since_epoch();

//TODO: std::vector<std::string> translate_keybinding_id(uint64_t id);

} // namespace ura::api::core

namespace ura::api::lua {
void print(sol::variadic_args va);
} // namespace ura::api::lua
