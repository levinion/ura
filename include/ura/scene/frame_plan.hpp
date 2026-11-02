#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "ura/ura.hpp"

namespace ura {

struct FrameSurface {
  wlr_surface* surface = nullptr;
  wlr_texture* texture = nullptr;
  wlr_fbox src_box {};
  wlr_box dst_box {};
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  float opacity = 1.f;
  wlr_drm_syncobj_timeline* wait_timeline = nullptr;
  uint64_t wait_point = 0;
  // Frame callbacks are completed by a single output only.
  bool primary_output = false;
};

struct FrameRect {
  wlr_box dst_box {};
  std::array<float, 4> color { { 0.f, 0.f, 0.f, 1.f } };
  uint64_t owner = 0;
  int index = 0;
};

enum class FrameOperationKind {
  Surface,
  Rect,
};

struct FrameOperation {
  FrameOperationKind kind = FrameOperationKind::Surface;
  std::size_t index = 0;
};

struct FramePlan {
  wlr_output* output = nullptr;
  wlr_damage_ring* damage_ring = nullptr;
  wlr_box logical_box {};
  std::array<float, 4> background_color { { 0.f, 0.f, 0.f, 1.f } };
  std::vector<FrameSurface> surfaces;
  std::vector<FrameRect> rects;
  // Rendering order for surfaces and rectangles.
  std::vector<FrameOperation> operations;
  // Damage rectangles in output-layout coordinates. Empty means no redraw.
  std::vector<wlr_box> damage;
  bool full_redraw = false;
  // Set only for scene trees that have no additional compositor drawing
  // (borders, future scene rects, ...) outside the surface list.
  bool allow_direct_scanout = false;
};

} // namespace ura
