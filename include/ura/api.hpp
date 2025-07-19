#pragma once

#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <string>

namespace ura {

void map(std::string modifiers, std::string key, sol::protected_function f);
void terminate();
void close_window();
void fullscreen();
void set_output_scale(float scale);
void set_output_refresh(int refresh);
void reload();
void set_keyboard_repeat(int rate, int delay);
void focus_follow_mouse(bool flag);
void env(std::string name, std::string value);
void switch_workspace(int index);
void move_to_workspace(int index);
int current_workspace();
void hook(std::string, sol::protected_function f);
void tiling_gap(float outer, float inner);
void cursor_theme(std::string theme, int size);

} // namespace ura
