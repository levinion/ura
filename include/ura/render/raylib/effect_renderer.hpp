#pragma once

#include <cstddef>

namespace ura {
struct EffectInstance;
}

struct wlr_render_pass;
struct wlr_render_texture_options;
struct wlr_render_rect_options;

namespace ura::render::raylib {

struct EffectGroupItem {
  const wlr_render_texture_options* texture = nullptr;
  const wlr_render_rect_options* rect = nullptr;
};

bool render_pass_add_texture_effect(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options,
  const EffectInstance& effect
);

bool render_pass_add_texture_group_effect(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options,
  std::size_t count,
  const EffectInstance& effect
);

// Applies one effect to a SceneNode composition that may contain both client
// textures (root/subsurfaces/CSD) and solid rectangles (borders).
bool render_pass_add_group_effect(
  wlr_render_pass* pass,
  const EffectGroupItem* items,
  std::size_t count,
  const EffectInstance& effect,
  float scale = 1.0f,
  float rotation = 0.0f,
  double rotation_center_x = 0.0,
  double rotation_center_y = 0.0
);

} // namespace ura::render::raylib
