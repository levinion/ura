#pragma once

#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura {

void map(std::string modifiers, std::string key, sol::protected_function f);
void terminate();
void close_window();
void toggle_fullscreen();
void reload();
void set_keyboard_repeat(int rate, int delay);
void focus_follow_mouse(bool flag);
void env(std::string name, std::string value);
int switch_workspace(int index);
int move_to_workspace(int index);
int current_workspace();
void hook(std::string, sol::protected_function f);
void tiling_gap(
  float inner,
  float outer_l,
  float outer_r,
  float outer_t,
  float outer_b
);
void set_cursor_theme(std::string theme, int size);
void hide_cursor();
void show_cursor();
void cursor_absolute_move(int x, int y);
void cursor_relative_move(int delta_x, int delta_y);
void set_cursor_shape(std::string name);
int current_toplevel();
bool focus(int index);
void toggle_float();
void set_default_floating_size(int width, int height);
// void set_floating_size(int width, int height);
// get_floating_size();
} // namespace ura
