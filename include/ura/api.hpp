#pragma once

#include <sol/sol.hpp>
#include <string>

namespace ura {

void map(std::string modifiers, std::string key, sol::protected_function func);
void terminate();
void close_window();
void fullscreen();
void set_output_scale(float scale);
void reload();
void set_keyboard_repeat(int rate, int delay);
void focus_follow_mouse(bool flag);
void env(std::string name, std::string value);

} // namespace ura
