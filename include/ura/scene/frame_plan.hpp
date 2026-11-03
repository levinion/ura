#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "ura/render/effect.hpp"
#include "ura/ura.hpp"

namespace ura {

class UraOutput;

enum class FrameOperationKind {
  Surface,
  Rect,
};

struct FrameSurface {
  wlr_surface* surface = nullptr;
  wlr_texture* texture = nullptr;
  wlr_fbox src_box {};
  wlr_box dst_box {};
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  float opacity = 1.f;
  wlr_drm_syncobj_timeline* wait_timeline = nullptr;
  uint64_t wait_point = 0;
  uint32_t seq = 0;
  // Stable identity for surfaces without a wlr_surface (frozen render
  // targets). Zero means the identity is the wlr_surface pointer.
  uint64_t content_id = 0;
  EffectInstance effect;
  // Whole-SceneNode rotation in radians, around this point in output-layout
  // coordinates.
  float rotation = 0.f;
  double rotation_center_x = 0.0;
  double rotation_center_y = 0.0;
  // SceneNode identity used to group multiple surfaces into one effect pass.
  uint64_t owner = 0;
  // Frame callbacks are completed by a single output only.
  bool primary_output = false;
};

struct FrameRect {
  wlr_box dst_box {};
  std::array<float, 4> color { { 0.f, 0.f, 0.f, 1.f } };
  uint64_t owner = 0;
  int index = 0;
  EffectInstance effect;
  float rotation = 0.f;
  double rotation_center_x = 0.0;
  double rotation_center_y = 0.0;
};

struct FrameSurfaceSnapshot {
  wlr_surface* surface = nullptr;
  wlr_texture* texture = nullptr;
  wlr_fbox src_box {};
  wlr_box dst_box {};
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  float opacity = 1.f;
  wlr_drm_syncobj_timeline* wait_timeline = nullptr;
  uint64_t wait_point = 0;
  uint32_t seq = 0;
  std::string_view output_name;
  bool lock_surface = false;
  bool allow_direct_scanout = false;
  uint64_t content_id = 0;
  EffectInstance effect;
  float rotation = 0.f;
  double rotation_center_x = 0.0;
  double rotation_center_y = 0.0;
  // SceneNode identity used to group multiple surfaces into one effect pass.
  uint64_t owner = 0;
};

struct FrameRectSnapshot {
  FrameRect rect;
  std::string_view output_name;
};

struct FrameOperationSnapshot {
  FrameOperationKind kind = FrameOperationKind::Surface;
  std::size_t index = 0;
};

struct FrameWorldSnapshot {
  bool locked = false;
  std::size_t tasks = 0;
  std::vector<FrameSurfaceSnapshot> surfaces;
  std::vector<FrameRectSnapshot> rects;
  std::vector<FrameOperationSnapshot> operations;
};

struct OutputFrameState {
  UraOutput* output = nullptr;
  wlr_output* native = nullptr;
  bool enabled = false;
  std::string_view name;
  wlr_box logical_box {};
  float scale = 1.f;
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  bool software_cursor = false;
  wlr_damage_ring* damage_ring = nullptr;
  std::array<float, 4> background_color { { 0.f, 0.f, 0.f, 1.f } };
};

struct FrameOperation {
  FrameOperationKind kind = FrameOperationKind::Surface;
  std::size_t index = 0;
};

struct FramePlan {
  wlr_output* output = nullptr;
  wlr_damage_ring* damage_ring = nullptr;
  wlr_drm_syncobj_timeline* render_timeline = nullptr;
  uint64_t render_point = 0;
  wlr_box logical_box {};
  std::array<float, 4> background_color { { 0.f, 0.f, 0.f, 1.f } };
  std::vector<FrameSurface> surfaces;
  std::vector<FrameRect> rects;
  // Rendering order for surfaces and rectangles.
  std::vector<FrameOperation> operations;
  // Damage rectangles in output-layout coordinates. Empty means no redraw.
  std::vector<wlr_box> damage;
  bool full_redraw = false;
  // Renderer-owned slots used to keep the previous two swapchain buffers
  // alive. May be NULL in tests or non-output clients.
  std::array<wlr_buffer*, 2>* retained_buffers = nullptr;
  // Set only for scene trees that have no additional compositor drawing
  // (borders, future scene rects, ...) outside the surface list.
  bool allow_direct_scanout = false;
};

} // namespace ura
