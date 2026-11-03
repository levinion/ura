#include <cstdio>

#include "ura/render/frame_renderer.hpp"
#include "ura/render/raylib/raylib_renderer.hpp"

extern "C" {
#include <drm_fourcc.h>
#include <wlr/backend/headless.h>
#include <wlr/render/drm_syncobj.h>
}

int main() {
  wlr_log_init(WLR_INFO, nullptr);
  wl_display* display = wl_display_create();
  if (!display)
    return 1;

  wl_event_loop* loop = wl_display_get_event_loop(display);
  wlr_backend* backend = wlr_headless_backend_create(loop);
  if (!backend) {
    wl_display_destroy(display);
    return 77;
  }

  wlr_renderer* renderer = ura_raylib_renderer_create(backend);
  if (!renderer) {
    fprintf(stderr, "skipping: raylib renderer could not be created\n");
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 77;
  }

  wlr_allocator* allocator = wlr_allocator_autocreate(backend, renderer);
  if (!allocator) {
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  wlr_output* output = wlr_headless_add_output(backend, 320, 200);
  if (!output || !wlr_output_init_render(output, allocator, renderer)) {
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  wlr_output_state state {};
  wlr_output_state_set_enabled(&state, true);
  wlr_output_state_set_custom_mode(&state, 320, 200, 0);
  if (!wlr_output_commit_state(output, &state)) {
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  ura::FramePlan plan;
  plan.output = output;
  plan.logical_box = { 0, 0, 320, 200 };
  plan.background_color = { 0.1f, 0.2f, 0.3f, 1.f };
  plan.full_redraw = true;

  wlr_drm_syncobj_timeline* timeline = nullptr;
  int drm_fd = wlr_renderer_get_drm_fd(renderer);
  if (drm_fd >= 0)
    timeline = wlr_drm_syncobj_timeline_create(drm_fd);
  plan.render_timeline = timeline;
  plan.render_point = timeline ? 1 : 0;

  bool ok = ura::render_frame(renderer, plan);
  if (ok) {
    plan.full_redraw = false;
    plan.damage = { { 16, 16, 32, 32 } };
    plan.rects.push_back(
      ura::FrameRect {
        .dst_box = { 16, 16, 32, 2 },
        .color = { 1.f, 0.f, 0.f, 1.f },
        .owner = 1,
        .index = 0,
        .effect = {},
      }
    );
    plan.operations.push_back(
      ura::FrameOperation {
        .kind = ura::FrameOperationKind::Rect,
        .index = 0,
      }
    );
    if (timeline)
      plan.render_point = 2;
    ok = ura::render_frame(renderer, plan);
  }

  // A frozen SceneNode has no wlr_surface. Rendering must still sample its
  // renderer-owned texture and skip surface-only presentation/frame callbacks.
  if (ok) {
    uint32_t pixels[16] = {};
    for (auto& pixel : pixels) pixel = 0xFFFF0000u;
    auto* frozen = wlr_texture_from_pixels(
      renderer,
      DRM_FORMAT_ARGB8888,
      4 * sizeof(uint32_t),
      4,
      4,
      pixels
    );
    if (!frozen)
      return 1;

    plan.surfaces.clear();
    plan.rects.clear();
    plan.operations.clear();
    plan.damage.clear();
    plan.full_redraw = true;
    plan.surfaces.push_back(
      ura::FrameSurface {
        .surface = nullptr,
        .texture = frozen,
        .src_box = { 0, 0, 4, 4 },
        .dst_box = { 40, 40, 80, 80 },
        .transform = WL_OUTPUT_TRANSFORM_NORMAL,
        .content_id = 1,
        .effect = {},
      }
    );
    plan.operations.push_back(
      ura::FrameOperation {
        .kind = ura::FrameOperationKind::Surface,
        .index = 0,
      }
    );
    if (timeline)
      plan.render_point = 3;
    ok = ura::render_frame(renderer, plan);

    // Generic single-pass effect through the full frame renderer, including
    // the same-owner border rectangle.
    if (ok) {
      plan.rects.push_back(
        ura::FrameRect {
          .dst_box = { 40, 40, 80, 2 },
          .color = { 1.f, 0.f, 0.f, 1.f },
          .owner = 1,
          .index = 0,
          .effect = {},
        }
      );
      plan.operations.push_back(
        ura::FrameOperation {
          .kind = ura::FrameOperationKind::Rect,
          .index = 0,
        }
      );
      auto pipeline = std::make_shared<ura::EffectPipeline>();
      pipeline->handle = ura::EffectHandle { .id = 11, .generation = 1 };
      pipeline->name = "frame-effect";
      pipeline->passes.push_back(
        ura::EffectPass {
          .id = {},
          .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
void main() {
    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)",
          .inputs = {},
          .uniforms = {},
        }
      );
      ura::EffectInstance instance;
      instance.pipeline = pipeline;
      instance.version = 1;
      instance.outset = { .left = 8, .top = 8, .right = 8, .bottom = 8 };
      auto effect = instance;
      plan.surfaces[0].effect = effect;
      plan.rects[0].effect = effect;
      plan.full_redraw = true;
      if (timeline)
        plan.render_point = 6;
      ok = ura::render_frame(renderer, plan);
    }

    wlr_texture_destroy(frozen);
  }

  if (timeline) {
    wlr_drm_syncobj_timeline_signal(timeline, UINT64_MAX);
    wlr_drm_syncobj_timeline_unref(timeline);
  }
  wlr_allocator_destroy(allocator);
  wlr_renderer_destroy(renderer);
  wlr_backend_destroy(backend);
  wl_display_destroy(display);
  return ok ? 0 : 1;
}
