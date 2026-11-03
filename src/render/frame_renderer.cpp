#include "ura/render/frame_renderer.hpp"
#include "ura/render/raylib/effect_renderer.hpp"

#include <cmath>
#include <cstdlib>
#include <optional>
#include <vector>

#include "ura/ura.hpp"

extern "C" {
#include <wlr/render/swapchain.h>
#include <wlr/util/region.h>
#include <wlr/util/transform.h>
}

namespace ura {

namespace {

  int scale_length(int length, int offset, float scale) {
    return static_cast<int>(std::round((offset + length) * scale))
      - static_cast<int>(std::round(offset * scale));
  }

  void scale_box(wlr_box* box, float scale) {
    box->width = scale_length(box->width, box->x, scale);
    box->height = scale_length(box->height, box->y, scale);
    box->x = static_cast<int>(std::round(box->x * scale));
    box->y = static_cast<int>(std::round(box->y * scale));
  }

  bool same_box(const wlr_box& lhs, const wlr_box& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width
      && lhs.height == rhs.height;
  }

  bool has_software_cursor(wlr_output* output) {
    wlr_output_cursor* cursor;
    wl_list_for_each(cursor, &output->cursors, link) {
      if (
        cursor->enabled && cursor->visible && cursor != output->hardware_cursor
      )
        return true;
    }
    return false;
  }

  bool try_direct_scanout(const FramePlan& plan) {
    auto* output = plan.output;
    if (!plan.allow_direct_scanout || !output)
      return false;
    if (!wlr_output_is_direct_scanout_allowed(output))
      return false;
    if (plan.surfaces.size() != 1 || !plan.rects.empty())
      return false;

    auto& item = plan.surfaces[0];
    auto* surface = item.surface;
    if (
      !surface || !surface->buffer
      || !wlr_buffer_is_opaque(&surface->buffer->base)
    )
      return false;
    if (item.opacity != 1.f)
      return false;
    if (item.effect.active() || item.rotation != 0.f)
      return false;
    if (surface->current.viewport.has_src || surface->current.viewport.has_dst)
      return false;
    if (item.transform != output->transform)
      return false;
    if (!same_box(item.dst_box, plan.logical_box))
      return false;
    if (has_software_cursor(output))
      return false;
    if (getenv("URA_RAYLIB_EFFECT_TINT"))
      return false;

    auto* buffer = surface->buffer->source ? surface->buffer->source
                                           : &surface->buffer->base;

    wlr_output_state state {};
    wlr_output_state_init(&state);
    wlr_output_state_set_buffer(&state, buffer);
    if (item.wait_timeline) {
      wlr_output_state_set_wait_timeline(
        &state,
        item.wait_timeline,
        item.wait_point
      );
    }

    pixman_region32_t damage;
    pixman_region32_init_rect(&damage, 0, 0, buffer->width, buffer->height);
    wlr_output_state_set_damage(&state, &damage);

    bool ok = wlr_output_test_state(output, &state);
    if (ok) {
      wlr_presentation_surface_scanned_out_on_output(surface, output);
      ok = wlr_output_commit_state(output, &state);
    }
    if (ok && plan.damage_ring)
      wlr_damage_ring_add_whole(plan.damage_ring);

    pixman_region32_fini(&damage);
    wlr_output_state_finish(&state);
    return ok;
  }

  void output_box(
    const FramePlan& plan,
    int resolution_width,
    int resolution_height,
    wlr_box* box
  ) {
    auto* output = plan.output;
    int trans_width = resolution_width;
    int trans_height = resolution_height;
    wlr_output_transform_coords(output->transform, &trans_width, &trans_height);

    box->x -= plan.logical_box.x;
    box->y -= plan.logical_box.y;
    auto transform = wlr_output_transform_invert(output->transform);
    scale_box(box, output->scale);
    wlr_box_transform(box, box, transform, trans_width, trans_height);
  }

} // namespace

bool render_frame(wlr_renderer* renderer, const FramePlan& plan) {
  auto* wlr_output = plan.output;
  if (!renderer || !wlr_output || !wlr_output->enabled)
    return false;
  if (plan.logical_box.width <= 0 || plan.logical_box.height <= 0)
    return false;
  if (try_direct_scanout(plan))
    return true;
  wlr_output_state state {};
  wlr_output_state_init(&state);

  if (!wlr_output_configure_primary_swapchain(
        wlr_output,
        &state,
        &wlr_output->swapchain
      )) {
    wlr_output_state_finish(&state);
    return false;
  }

  auto* buffer = wlr_swapchain_acquire(wlr_output->swapchain);
  if (!buffer) {
    wlr_output_state_finish(&state);
    return false;
  }

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  if (plan.full_redraw) {
    pixman_region32_init_rect(&damage, 0, 0, buffer->width, buffer->height);
  } else {
    for (auto& box : plan.damage) {
      wlr_box dst = box;
      output_box(plan, buffer->width, buffer->height, &dst);
      if (dst.width <= 0 || dst.height <= 0)
        continue;
      pixman_region32_union_rect(
        &damage,
        &damage,
        dst.x,
        dst.y,
        dst.width,
        dst.height
      );
    }

    pixman_region32_t bounds;
    pixman_region32_init_rect(&bounds, 0, 0, buffer->width, buffer->height);
    pixman_region32_intersect(&damage, &damage, &bounds);
    pixman_region32_fini(&bounds);
  }

  if (plan.damage_ring) {
    wlr_damage_ring_add(plan.damage_ring, &damage);
    pixman_region32_t ring_damage;
    pixman_region32_init(&ring_damage);
    wlr_damage_ring_rotate_buffer(plan.damage_ring, buffer, &ring_damage);
    pixman_region32_fini(&damage);
    pixman_region32_init(&damage);
    pixman_region32_copy(&damage, &ring_damage);
    pixman_region32_fini(&ring_damage);
  }

  if (pixman_region32_empty(&damage)) {
    pixman_region32_fini(&damage);
    wlr_buffer_unlock(buffer);
    wlr_output_state_finish(&state);
    return false;
  }

  wlr_output_state_set_buffer(&state, buffer);
  wlr_output_state_set_damage(&state, &damage);

  bool use_timeline = plan.render_timeline && plan.render_point != 0;
  if (use_timeline) {
    wlr_output_state test {};
    wlr_output_state_init(&test);
    if (wlr_output_state_copy(&test, &state)) {
      wlr_output_state_set_wait_timeline(
        &test,
        plan.render_timeline,
        plan.render_point
      );
      use_timeline = wlr_output_test_state(wlr_output, &test);
    } else {
      use_timeline = false;
    }
    wlr_output_state_finish(&test);
  }

  wlr_buffer_pass_options pass_options {};
  if (use_timeline) {
    pass_options.signal_timeline = plan.render_timeline;
    pass_options.signal_point = plan.render_point;
  }

  auto* pass = wlr_renderer_begin_buffer_pass(renderer, buffer, &pass_options);
  if (!pass) {
    if (plan.damage_ring)
      wlr_damage_ring_add_whole(plan.damage_ring);
    pixman_region32_fini(&damage);
    wlr_buffer_unlock(buffer);
    wlr_output_state_finish(&state);
    return false;
  }

  wlr_render_rect_options background {
    .box = {
      .x = 0,
      .y = 0,
      .width = buffer->width,
      .height = buffer->height,
    },
    .color = {
      .r = plan.background_color[0],
      .g = plan.background_color[1],
      .b = plan.background_color[2],
      .a = plan.background_color[3],
    },
    .clip = &damage,
    .blend_mode = WLR_RENDER_BLEND_MODE_NONE,
  };
  wlr_render_pass_add_rect(pass, &background);

  auto make_texture_options =
    [&](const FrameSurface& item) -> std::optional<wlr_render_texture_options> {
    wlr_box dst = item.dst_box;
    output_box(plan, buffer->width, buffer->height, &dst);

    wlr_box cull = dst;
    if (item.rotation == 0.f && item.effect.active()) {
      auto outset = item.effect.outset;
      auto scaled = [&](int value) {
        return static_cast<int>(
          std::ceil(static_cast<float>(value) * wlr_output->scale)
        );
      };
      auto left = scaled(outset.left);
      auto top = scaled(outset.top);
      auto right = scaled(outset.right);
      auto bottom = scaled(outset.bottom);
      cull.x -= left;
      cull.y -= top;
      cull.width += left + right;
      cull.height += top + bottom;
    }

    pixman_region32_t intersection;
    pixman_region32_init(&intersection);
    pixman_region32_intersect_rect(
      &intersection,
      &damage,
      cull.x,
      cull.y,
      cull.width,
      cull.height
    );
    if (pixman_region32_empty(&intersection)) {
      pixman_region32_fini(&intersection);
      return {};
    }
    pixman_region32_fini(&intersection);

    auto transform = wlr_output_transform_invert(item.transform);
    transform = wlr_output_transform_compose(transform, wlr_output->transform);

    return wlr_render_texture_options {
      .texture = item.texture,
      .src_box = item.src_box,
      .dst_box = dst,
      .alpha = &item.opacity,
      .clip = &damage,
      .transform = transform,
      .filter_mode = WLR_SCALE_FILTER_BILINEAR,
      .blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED,
      .transfer_function = WLR_COLOR_TRANSFER_FUNCTION_GAMMA22,
      .primaries = nullptr,
      .color_encoding = WLR_COLOR_ENCODING_NONE,
      .color_range = WLR_COLOR_RANGE_NONE,
      .luminance_multiplier = nullptr,
      .wait_timeline = item.wait_timeline,
      .wait_point = item.wait_point,
    };
  };

  for (std::size_t i = 0; i < plan.operations.size();) {
    auto& operation = plan.operations[i];
    if (operation.kind == FrameOperationKind::Rect) {
      auto& rect = plan.rects[operation.index];
      wlr_box dst = rect.dst_box;
      output_box(plan, buffer->width, buffer->height, &dst);
      wlr_render_rect_options options {
        .box = dst,
        .color = {
          .r = rect.color[0],
          .g = rect.color[1],
          .b = rect.color[2],
          .a = rect.color[3],
        },
        .clip = &damage,
        .blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED,
      };
      wlr_render_pass_add_rect(pass, &options);
      ++i;
      continue;
    }

    auto& first = plan.surfaces[operation.index];
    if (!first.effect.active() && first.rotation == 0.f) {
      auto options = make_texture_options(first);
      if (options) {
        wlr_render_pass_add_texture(pass, &*options);
        if (first.surface)
          wlr_presentation_surface_textured_on_output(
            first.surface,
            wlr_output
          );
      }
      ++i;
      continue;
    }

    // Everything belonging to this SceneNode is composed into one offscreen
    // target before the effect runs. This includes borders represented as
    // FrameRect operations, not just the client surface tree.
    std::vector<wlr_render_texture_options> group_textures;
    std::vector<wlr_render_rect_options> group_rects;
    std::vector<render::raylib::EffectGroupItem> group_items;
    std::vector<const FrameSurface*> group_surfaces;
    auto capacity = plan.operations.size() - i;
    group_textures.reserve(capacity);
    group_rects.reserve(capacity);
    group_items.reserve(capacity);
    group_surfaces.reserve(capacity);

    auto make_rect_options = [&](const FrameRect& rect) {
      wlr_box dst = rect.dst_box;
      output_box(plan, buffer->width, buffer->height, &dst);
      return wlr_render_rect_options {
        .box = dst,
        .color = {
          .r = rect.color[0],
          .g = rect.color[1],
          .b = rect.color[2],
          .a = rect.color[3],
        },
        .clip = &damage,
        .blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED,
      };
    };

    std::size_t j = i;
    while (j < plan.operations.size()) {
      auto& next_operation = plan.operations[j];
      if (next_operation.kind == FrameOperationKind::Rect) {
        auto& rect = plan.rects[next_operation.index];
        if (rect.owner != first.owner)
          break;
        auto options = make_rect_options(rect);
        group_rects.push_back(options);
        group_items.push_back(
          render::raylib::EffectGroupItem { .rect = &group_rects.back() }
        );
        ++j;
        continue;
      }

      auto& item = plan.surfaces[next_operation.index];
      if (item.owner != first.owner || item.effect != first.effect)
        break;
      auto options = make_texture_options(item);
      if (options) {
        group_textures.push_back(*options);
        group_items.push_back(
          render::raylib::EffectGroupItem { .texture = &group_textures.back() }
        );
        group_surfaces.push_back(&item);
      }
      ++j;
    }

    if (!group_items.empty()) {
      wlr_box rotation_center {
        .x = static_cast<int>(std::lround(first.rotation_center_x)),
        .y = static_cast<int>(std::lround(first.rotation_center_y)),
        .width = 1,
        .height = 1,
      };
      output_box(plan, buffer->width, buffer->height, &rotation_center);
      bool rendered = render::raylib::render_pass_add_group_effect(
        pass,
        group_items.data(),
        group_items.size(),
        first.effect,
        wlr_output->scale,
        first.rotation,
        static_cast<double>(rotation_center.x),
        static_cast<double>(rotation_center.y)
      );
      if (!rendered) {
        for (auto& item : group_items) {
          if (item.texture)
            wlr_render_pass_add_texture(pass, item.texture);
          else if (item.rect)
            wlr_render_pass_add_rect(pass, item.rect);
        }
      }
      for (auto* item : group_surfaces) {
        if (item->surface)
          wlr_presentation_surface_textured_on_output(
            item->surface,
            wlr_output
          );
      }
    }
    i = j;
  }

  wlr_output_add_software_cursors_to_render_pass(wlr_output, pass, &damage);

  if (!wlr_render_pass_submit(pass)) {
    if (plan.damage_ring)
      wlr_damage_ring_add_whole(plan.damage_ring);
    pixman_region32_fini(&damage);
    wlr_buffer_unlock(buffer);
    wlr_output_state_finish(&state);
    return false;
  }

  if (use_timeline) {
    wlr_output_state_set_wait_timeline(
      &state,
      plan.render_timeline,
      plan.render_point
    );
  }
  wlr_buffer_unlock(buffer);

  bool ok = wlr_output_commit_state(wlr_output, &state);
  if (ok && plan.retained_buffers) {
    // Keep the last two presented buffers locked. With lazy swapchain
    // allocation this forces a three-buffer rotation, giving drivers that
    // still read a released buffer (NVIDIA + DSC) enough time to finish.
    auto& retained = *plan.retained_buffers;
    if (buffer != retained[0] && buffer != retained[1]) {
      if (retained[1])
        wlr_buffer_unlock(retained[1]);
      retained[1] = retained[0];
      retained[0] = wlr_buffer_lock(buffer);
    }
  } else if (!ok && plan.damage_ring) {
    wlr_damage_ring_add_whole(plan.damage_ring);
  }
  pixman_region32_fini(&damage);
  wlr_output_state_finish(&state);
  return ok;
}

} // namespace ura
