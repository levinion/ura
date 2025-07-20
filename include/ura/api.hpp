#pragma once

#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura {

void map(std::string modifiers, std::string key, sol::protected_function f);
void terminate();
void close_window();
void fullscreen();
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
void cursor_theme(std::string theme, int size);
int current_toplevel();
bool focus(int index);

} // namespace ura
