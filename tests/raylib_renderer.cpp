#include <cstdio>
#include <cstdlib>
#include <cstring>

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

  wlr_scene* scene = wlr_scene_create();
  wlr_scene_output* scene_output = wlr_scene_output_create(scene, output);
  if (!scene || !scene_output) {
    fprintf(stderr, "failed to create scene output\n");
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(display);
    return 1;
  }

  const float background[4] = { 0.1f, 0.2f, 0.3f, 1.0f };
  wlr_scene_rect_create(&scene->tree, 320, 200, background);

  TestBuffer* source = create_test_buffer(32, 32, 0xFFFF0000);
  if (!source) {
    fprintf(stderr, "failed to create test buffer\n");
    return 1;
  }
  wlr_scene_buffer* scene_buffer =
    wlr_scene_buffer_create(&scene->tree, &source->base);
  wlr_scene_node_set_position(&scene_buffer->node, 16, 16);

  bool ok = wlr_scene_output_commit(scene_output, nullptr);
  if (!ok) {
    fprintf(stderr, "scene output commit failed\n");
    return 1;
  }
  ok = wlr_scene_output_commit(scene_output, nullptr);
  if (!ok) {
    fprintf(stderr, "second scene output commit failed\n");
    return 1;
  }

  // Exercise texture import and the render pass API directly, including the
  // SHM render-target readback path. This is independent from wlr_scene.
  TestBuffer* target = create_test_buffer(64, 64, 0);
  wlr_texture* texture = wlr_texture_from_buffer(renderer, &source->base);
  if (!target || !texture) {
    fprintf(stderr, "failed to create texture or render target\n");
    return 1;
  }

  wlr_render_pass* pass =
    wlr_renderer_begin_buffer_pass(renderer, &target->base, nullptr);
  if (!pass) {
    fprintf(stderr, "failed to begin render pass\n");
    return 1;
  }

  wlr_render_rect_options rect_options {};
  rect_options.box = { .x = 0, .y = 0, .width = 64, .height = 64 };
  rect_options.color = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
  rect_options.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(pass, &rect_options);

  wlr_drm_syncobj_timeline* wait_timeline = nullptr;
  int drm_fd = wlr_renderer_get_drm_fd(renderer);
  if (drm_fd >= 0) {
    wait_timeline = wlr_drm_syncobj_timeline_create(drm_fd);
    if (wait_timeline)
      wlr_drm_syncobj_timeline_signal(wait_timeline, 1);
  }

  wlr_render_texture_options texture_options {};
  texture_options.texture = texture;
  texture_options.dst_box = { .x = 8, .y = 8, .width = 16, .height = 16 };
  texture_options.transform = WL_OUTPUT_TRANSFORM_NORMAL;
  texture_options.filter_mode = WLR_SCALE_FILTER_BILINEAR;
  texture_options.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED;
  texture_options.wait_timeline = wait_timeline;
  texture_options.wait_point = 1;
  wlr_render_pass_add_texture(pass, &texture_options);

  if (!wlr_render_pass_submit(pass)) {
    fprintf(stderr, "render pass submit failed\n");
    return 1;
  }

  uint32_t white = pixel_at(target, 0, 0);
  uint32_t red = pixel_at(target, 12, 12);
  if (white != 0xFFFFFFFFu) {
    fprintf(stderr, "unexpected background pixel: 0x%08x\n", white);
    return 1;
  }
  if (red != 0xFFFF0000u) {
    fprintf(stderr, "unexpected textured pixel: 0x%08x\n", red);
    return 1;
  }

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
  wlr_scene_output_destroy(scene_output);
  wlr_scene_node_destroy(&scene->tree.node);
  wlr_buffer_drop(&source->base);
  wlr_output_destroy(output);
  wlr_allocator_destroy(allocator);
  wlr_renderer_destroy(renderer);
  wlr_backend_destroy(backend);
  wl_display_destroy(display);

  printf("raylib renderer: ok\n");
  return 0;
}
