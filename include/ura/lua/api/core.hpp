#pragma once

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
int get_window_number();
void move_window_to_workspace(uint64_t id, uint64_t workspace_id);
uint64_t get_current_window();
uint64_t get_window(int index);
void focus_window(uint64_t id);
void set_window_z_index(uint64_t id, int z);
void set_window_draggable(uint64_t id, bool flag);
int get_window_z_index(uint64_t id);
bool is_window_draggable(uint64_t id);
void activate_window(uint64_t id);
void move_window(uint64_t id, int x, int y);
void resize_window(uint64_t id, int width, int height);
void swap_window(uint64_t id, uint64_t target);
// input
void set_keyboard_repeat(int rate, int delay);
void set_pointer_properties(std::string pattern, flexible::object object);
void set_cursor_theme(std::string theme, int size);
std::string get_cursor_theme();
int get_cursor_size();
void set_cursor_visible(bool flag);
bool is_cursor_visible();
void set_cursor_shape(std::string name);
std::string get_cursor_shape();
// ws
void create_workspace();
void switch_workspace(uint64_t id);
void destroy_workspace(uint64_t id);
int get_workspace_number();
uint64_t get_current_workspace();
uint64_t get_workspace(flexible::object obj);
// output
uint64_t get_current_output();
uint64_t get_output(std::string name);
void set_output_dpms(uint64_t id, bool flag);
// fn
void set_env(std::string name, std::string value);
void unset_env(std::string name);

// TODO: impl in lua
// void append_lua_package_path(std::string path);
// void prepend_lua_package_path(std::string path);
// std::string expanduser(std::string path);
// std::string expandvars(std::string path);
// std::string expand(std::string path);
} // namespace ura::api::core
