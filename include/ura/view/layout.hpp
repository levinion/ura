#pragma once
#include "ura/lua/lua.hpp"

namespace ura::layout {
std::optional<sol::table> tiling(int index);
std::optional<sol::table> fullscreen(int index);
std::optional<sol::table> floating(int index);
} // namespace ura::layout
