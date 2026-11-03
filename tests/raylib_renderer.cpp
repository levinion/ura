#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ura/render/effect.hpp"
#include "ura/render/raylib/effect_renderer.hpp"
#include "ura/render/raylib/raylib_renderer.hpp"
#include "ura/ura.hpp"

extern "C" {
#include <drm_fourcc.h>
#include <wlr/backend/headless.h>
#include <wlr/interfaces/wlr_buffer.h>
#include <wlr/render/drm_syncobj.h>
}

namespace {

struct TestBuffer {
  wlr_buffer base;
  uint8_t* data;
  size_t stride;
  uint32_t format;
};

void test_buffer_destroy(wlr_buffer* buffer) {
  TestBuffer* test_buffer = wl_container_of(buffer, test_buffer, base);
  wlr_buffer_finish(buffer);
  free(test_buffer->data);
  free(test_buffer);
}

bool test_buffer_begin_data_ptr_access(
  wlr_buffer* buffer,
  uint32_t flags,
  void** data,
  uint32_t* format,
  size_t* stride
) {
  (void)flags;
  TestBuffer* test_buffer = wl_container_of(buffer, test_buffer, base);
  *data = test_buffer->data;
  *format = test_buffer->format;
  *stride = test_buffer->stride;
  return true;
}

void test_buffer_end_data_ptr_access(wlr_buffer* buffer) {
  (void)buffer;
}

const wlr_buffer_impl test_buffer_impl = {
  .destroy = test_buffer_destroy,
  .get_dmabuf = nullptr,
  .get_shm = nullptr,
  .begin_data_ptr_access = test_buffer_begin_data_ptr_access,
  .end_data_ptr_access = test_buffer_end_data_ptr_access,
};

TestBuffer* create_test_buffer(int width, int height, uint32_t color) {
  auto* buffer = static_cast<TestBuffer*>(calloc(1, sizeof(TestBuffer)));
  if (!buffer)
    return nullptr;
  buffer->stride = static_cast<size_t>(width) * 4;
  buffer->format = DRM_FORMAT_ARGB8888;
  buffer->data =
    static_cast<uint8_t*>(calloc(static_cast<size_t>(height), buffer->stride));
  if (!buffer->data) {
    free(buffer);
    return nullptr;
  }
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      memcpy(
        buffer->data + static_cast<size_t>(y) * buffer->stride
          + static_cast<size_t>(x) * 4,
        &color,
        sizeof(color)
      );
    }
  }
  wlr_buffer_init(&buffer->base, &test_buffer_impl, width, height);
  return buffer;
}

uint32_t pixel_at(TestBuffer* buffer, int x, int y) {
  uint32_t value = 0;
  memcpy(
    &value,
    buffer->data + static_cast<size_t>(y) * buffer->stride
      + static_cast<size_t>(x) * 4,
    sizeof(value)
  );
  return value;
}

} // namespace

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
    fprintf(stderr, "failed to create allocator\n");
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  wlr_output* output = wlr_headless_add_output(backend, 320, 200);
  if (!output || !wlr_output_init_render(output, allocator, renderer)) {
    fprintf(stderr, "failed to create headless output\n");
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  wlr_output_state output_state {};
  wlr_output_state_set_enabled(&output_state, true);
  wlr_output_state_set_custom_mode(&output_state, 320, 200, 0);
  if (!wlr_output_commit_state(output, &output_state)) {
    fprintf(stderr, "failed to enable headless output\n");
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  TestBuffer* source = create_test_buffer(32, 32, 0xFFFF0000);
  if (!source) {
    fprintf(stderr, "failed to create test buffer\n");
    return 1;
  }

  // Exercise texture import and the render pass API directly, including the
  // SHM render-target readback path.
  TestBuffer* target = create_test_buffer(64, 64, 0);
  wlr_texture* texture = wlr_texture_from_buffer(renderer, &source->base);
  if (!target || !texture) {
    fprintf(stderr, "failed to create texture or render target\n");
    return 1;
  }

  wlr_drm_syncobj_timeline* wait_timeline = nullptr;
  wlr_drm_syncobj_timeline* signal_timeline = nullptr;
  int drm_fd = wlr_renderer_get_drm_fd(renderer);
  if (drm_fd >= 0) {
    wait_timeline = wlr_drm_syncobj_timeline_create(drm_fd);
    if (wait_timeline)
      wlr_drm_syncobj_timeline_signal(wait_timeline, 1);
    signal_timeline = wlr_drm_syncobj_timeline_create(drm_fd);
  }

  wlr_buffer_pass_options pass_options {};
  if (signal_timeline) {
    pass_options.signal_timeline = signal_timeline;
    pass_options.signal_point = 1;
  }
  wlr_render_pass* pass =
    wlr_renderer_begin_buffer_pass(renderer, &target->base, &pass_options);
  if (!pass) {
    fprintf(stderr, "failed to begin render pass\n");
    return 1;
  }

  wlr_render_rect_options rect_options {};
  rect_options.box = { .x = 0, .y = 0, .width = 64, .height = 64 };
  rect_options.color = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
  rect_options.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(pass, &rect_options);

  wlr_render_texture_options texture_options {};
  texture_options.texture = texture;
  texture_options.dst_box = { .x = 8, .y = 8, .width = 16, .height = 16 };
  texture_options.transform = WL_OUTPUT_TRANSFORM_NORMAL;
  texture_options.filter_mode = WLR_SCALE_FILTER_BILINEAR;
  texture_options.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED;
  texture_options.wait_timeline = wait_timeline;
  texture_options.wait_point = 1;
  wlr_render_pass_add_texture(pass, &texture_options);

  // Run the same texture through the offscreen tint pipeline. This exercises
  // target allocation, the copy pass, and compositing into the SHM target.
  auto effect_options = texture_options;
  effect_options.dst_box = { .x = 32, .y = 32, .width = 16, .height = 16 };
  auto tint = ura::Effect {
    .type = ura::EffectType::Tint,
    .color = { 0.5f, 0.5f, 1.0f, 1.0f },
  };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &effect_options,
        tint
      )) {
    fprintf(stderr, "effect render pass failed\n");
    return 1;
  }

  // Separable blur uses two additional offscreen passes. A solid red source
  // must still come back red rather than an empty/transparent target.
  auto blur_options = texture_options;
  blur_options.dst_box = { .x = 32, .y = 16, .width = 16, .height = 16 };
  auto blur = ura::Effect {
    .type = ura::EffectType::Blur,
    .radius = 4.f,
  };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &blur_options,
        blur
      )) {
    fprintf(stderr, "blur effect render pass failed\n");
    return 1;
  }

  // Two surfaces of the same SceneNode go through one group target and one
  // effect application. This is what keeps CSD/subsurfaces from developing
  // independent blur seams.
  auto group_a = texture_options;
  group_a.dst_box = { .x = 8, .y = 48, .width = 16, .height = 16 };
  auto group_b = texture_options;
  group_b.dst_box = { .x = 32, .y = 48, .width = 16, .height = 16 };
  wlr_render_texture_options group_options[] = { group_a, group_b };
  auto group_tint = ura::Effect {
    .type = ura::EffectType::Tint,
    .color = { 0.5f, 0.5f, 1.0f, 1.0f },
  };
  if (!ura::render::raylib::render_pass_add_texture_group_effect(
        pass,
        group_options,
        2,
        group_tint
      )) {
    fprintf(stderr, "group effect render pass failed\n");
    return 1;
  }

  if (!wlr_render_pass_submit(pass)) {
    fprintf(stderr, "render pass submit failed\n");
    return 1;
  }

  uint32_t white = pixel_at(target, 0, 0);
  uint32_t red = pixel_at(target, 12, 12);
  uint32_t tinted = pixel_at(target, 36, 36);
  if (white != 0xFFFFFFFFu) {
    fprintf(stderr, "unexpected background pixel: 0x%08x\n", white);
    return 1;
  }
  if (red != 0xFFFF0000u) {
    fprintf(stderr, "unexpected textured pixel: 0x%08x\n", red);
    return 1;
  }
  auto tinted_red = (tinted >> 16) & 0xFFu;
  if (
    (tinted & 0xFF000000u) != 0xFF000000u || tinted_red < 0x70u
    || tinted_red > 0x90u || (tinted & 0x0000FFFFu) != 0
  ) {
    fprintf(stderr, "unexpected effect pixel: 0x%08x\n", tinted);
    return 1;
  }

  uint32_t blurred = pixel_at(target, 36, 20);
  if (blurred != 0xFFFF0000u) {
    fprintf(stderr, "unexpected blur pixel: 0x%08x\n", blurred);
    return 1;
  }

  for (auto point : { std::pair { 12, 52 }, std::pair { 36, 52 } }) {
    uint32_t grouped = pixel_at(target, point.first, point.second);
    auto grouped_red = (grouped >> 16) & 0xFFu;
    if (
      (grouped & 0xFF000000u) != 0xFF000000u || grouped_red < 0x70u
      || grouped_red > 0x90u || (grouped & 0x0000FFFFu) != 0
    ) {
      fprintf(stderr, "unexpected group effect pixel: 0x%08x\n", grouped);
      return 1;
    }
  }

  // A non-zero src_box must select the requested sub-rectangle of the
  // texture after the transform-matrix refactor (raymath uses a different
  // matrix multiply order than the old hand-written helper).
  TestBuffer* source_rect = create_test_buffer(4, 4, 0xFFFF0000);
  if (!source_rect) {
    fprintf(stderr, "failed to create source-rect buffer\n");
    return 1;
  }
  for (int y = 0; y < 4; y++) {
    for (int x = 2; x < 4; x++) {
      uint32_t blue = 0xFF0000FFu;
      memcpy(
        source_rect->data + static_cast<size_t>(y) * source_rect->stride
          + static_cast<size_t>(x) * 4,
        &blue,
        sizeof(blue)
      );
    }
  }
  TestBuffer* rect_target = create_test_buffer(32, 32, 0xFFFFFFFFu);
  wlr_texture* rect_texture =
    wlr_texture_from_buffer(renderer, &source_rect->base);
  if (!rect_target || !rect_texture) {
    fprintf(stderr, "failed to create source-rect texture/target\n");
    return 1;
  }
  wlr_render_pass* rect_pass =
    wlr_renderer_begin_buffer_pass(renderer, &rect_target->base, nullptr);
  if (!rect_pass) {
    fprintf(stderr, "failed to begin source-rect render pass\n");
    return 1;
  }
  wlr_render_texture_options rect_texture_options {};
  rect_texture_options.texture = rect_texture;
  rect_texture_options.src_box = { .x = 2, .y = 0, .width = 2, .height = 4 };
  rect_texture_options.dst_box = { .x = 8, .y = 8, .width = 16, .height = 16 };
  rect_texture_options.transform = WL_OUTPUT_TRANSFORM_NORMAL;
  rect_texture_options.filter_mode = WLR_SCALE_FILTER_NEAREST;
  rect_texture_options.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED;
  wlr_render_pass_add_texture(rect_pass, &rect_texture_options);
  if (!wlr_render_pass_submit(rect_pass)) {
    fprintf(stderr, "source-rect render pass submit failed\n");
    return 1;
  }
  if (pixel_at(rect_target, 16, 16) != 0xFF0000FFu) {
    fprintf(
      stderr,
      "unexpected source-rect pixel: 0x%08x\n",
      pixel_at(rect_target, 16, 16)
    );
    return 1;
  }
  wlr_texture_destroy(rect_texture);
  wlr_buffer_drop(&rect_target->base);
  wlr_buffer_drop(&source_rect->base);

  // Verify the optional rlgl effect path on a separate render target.
  setenv("URA_RAYLIB_EFFECT_TINT", "#00FF00FF", 1);
  TestBuffer* effect_target = create_test_buffer(16, 16, 0);
  if (!effect_target) {
    fprintf(stderr, "failed to create effect target\n");
    return 1;
  }
  wlr_render_pass* effect_pass =
    wlr_renderer_begin_buffer_pass(renderer, &effect_target->base, nullptr);
  if (!effect_pass) {
    fprintf(stderr, "failed to begin effect render pass\n");
    return 1;
  }
  wlr_render_rect_options effect_rect {};
  effect_rect.box = { .x = 0, .y = 0, .width = 16, .height = 16 };
  effect_rect.color = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
  effect_rect.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(effect_pass, &effect_rect);
  if (!wlr_render_pass_submit(effect_pass)) {
    fprintf(stderr, "effect render pass submit failed\n");
    return 1;
  }
  unsetenv("URA_RAYLIB_EFFECT_TINT");
  if (wait_timeline)
    wlr_drm_syncobj_timeline_unref(wait_timeline);
  if (signal_timeline) {
    wlr_drm_syncobj_timeline_signal(signal_timeline, UINT64_MAX);
    wlr_drm_syncobj_timeline_unref(signal_timeline);
  }
  if (pixel_at(effect_target, 0, 0) != 0xFF00FF00u) {
    fprintf(
      stderr,
      "unexpected effect pixel: 0x%08x\n",
      pixel_at(effect_target, 0, 0)
    );
    return 1;
  }
  wlr_buffer_drop(&effect_target->base);

  // Top/bottom orientation check. This catches a mismatch between the scene
  // projection, glScissor coordinates and SHM readback orientation.
  TestBuffer* orientation_target = create_test_buffer(16, 16, 0);
  if (!orientation_target) {
    fprintf(stderr, "failed to create orientation target\n");
    return 1;
  }
  wlr_render_pass* orientation_pass = wlr_renderer_begin_buffer_pass(
    renderer,
    &orientation_target->base,
    nullptr
  );
  if (!orientation_pass) {
    fprintf(stderr, "failed to begin orientation render pass\n");
    return 1;
  }
  wlr_render_rect_options top_rect {};
  top_rect.box = { .x = 0, .y = 0, .width = 16, .height = 8 };
  top_rect.color = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
  top_rect.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(orientation_pass, &top_rect);

  wlr_render_rect_options bottom_rect {};
  bottom_rect.box = { .x = 0, .y = 8, .width = 16, .height = 8 };
  bottom_rect.color = { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f };
  bottom_rect.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(orientation_pass, &bottom_rect);

  if (!wlr_render_pass_submit(orientation_pass)) {
    fprintf(stderr, "orientation render pass submit failed\n");
    return 1;
  }
  if (
    pixel_at(orientation_target, 0, 0) != 0xFFFF0000u
    || pixel_at(orientation_target, 0, 15) != 0xFF00FF00u
  ) {
    fprintf(
      stderr,
      "unexpected orientation pixels: top=0x%08x bottom=0x%08x\n",
      pixel_at(orientation_target, 0, 0),
      pixel_at(orientation_target, 0, 15)
    );
    return 1;
  }
  wlr_buffer_drop(&orientation_target->base);

  wlr_texture_destroy(texture);
  wlr_buffer_drop(&target->base);
  wlr_buffer_drop(&source->base);
  wlr_output_destroy(output);
  wlr_allocator_destroy(allocator);
  wlr_renderer_destroy(renderer);
  wlr_backend_destroy(backend);
  wl_display_destroy(display);

  printf("raylib renderer: ok\n");
  return 0;
}
