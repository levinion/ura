#pragma once

#include <cstddef>

namespace ura {
struct Effect;
}

struct wlr_render_pass;
struct wlr_render_texture_options;

namespace ura::render::raylib {

bool render_pass_add_texture_effect(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options,
  const Effect& effect
);

bool render_pass_add_texture_group_effect(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options,
  std::size_t count,
  const Effect& effect
);

} // namespace ura::render::raylib
