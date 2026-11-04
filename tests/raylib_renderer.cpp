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

  // zwlr-screencopy and ext-image-copy-capture advertise this format as a
  // wl_shm buffer format, so it must be one the wl_shm global also exposes.
  uint32_t read_format = wlr_texture_preferred_read_format(texture);
  const wlr_drm_format_set* shm_formats =
    wlr_renderer_get_texture_formats(renderer, WLR_BUFFER_CAP_SHM);
  if (
    read_format == DRM_FORMAT_INVALID || !shm_formats
    || !wlr_drm_format_set_has(shm_formats, read_format, DRM_FORMAT_MOD_INVALID)
  ) {
    fprintf(
      stderr,
      "preferred read format 0x%08x is not available for wl_shm\n",
      read_format
    );
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

  // Generic single-pass effect. This exercises the plugin-facing pipeline
  // path, shader compilation/cache, and typed vec4 uniform upload.
  auto pipeline = std::make_shared<ura::EffectPipeline>();
  pipeline->handle = ura::EffectHandle { .id = 1, .generation = 1 };
  pipeline->name = "test-color";
  pipeline->passes.push_back(
    ura::EffectPass {
      .id = {},
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec4 effectColor;
void main() {
    gl_FragColor = effectColor;
}
)",
      .inputs = {},
      .uniforms = {},
    }
  );
  pipeline->uniforms = {
    ura::EffectUniformDef {
      .name = "effectColor",
      .type = ura::EffectUniformType::Vec4,
    },
  };
  ura::EffectInstance instance;
  instance.pipeline = pipeline;
  instance.outset = {};
  instance.uniforms = {
    ura::EffectUniform {
      .name = "effectColor",
      .type = ura::EffectUniformType::Vec4,
      .values = { { 0.f, 1.f, 0.f, 1.f } },
    },
  };
  auto custom_effect = instance;
  auto custom_options = texture_options;
  custom_options.dst_box = { .x = 48, .y = 8, .width = 16, .height = 16 };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &custom_options,
        custom_effect
      )) {
    fprintf(stderr, "custom effect render pass failed\n");
    return 1;
  }

  // Two-pass generic effect: pass 1 copies the source, pass 2 emits blue.
  // This exercises intermediate target ping-pong and per-pass programs.
  auto multi_pipeline = std::make_shared<ura::EffectPipeline>();
  multi_pipeline->handle = ura::EffectHandle { .id = 2, .generation = 1 };
  multi_pipeline->name = "test-multipass";
  multi_pipeline->passes = {
    ura::EffectPass {
      .id = {},
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
void main() {
    gl_FragColor = texture2D(texture0, fragTexCoord);
}
)",
      .inputs = {},
      .uniforms = {},
    },
    ura::EffectPass {
      .id = {},
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 texelSize;
uniform vec4 passColor;
void main() {
    gl_FragColor = passColor;
}
)",
      .inputs = {},
      .uniforms = {
        ura::EffectUniform {
          .name = "passColor",
          .type = ura::EffectUniformType::Vec4,
          .values = { { 0.f, 0.f, 1.f, 1.f } },
        },
      },
    },
  };
  ura::EffectInstance multi_instance;
  multi_instance.pipeline = multi_pipeline;
  auto multi_effect = multi_instance;
  auto multi_options = texture_options;
  multi_options.dst_box = { .x = 48, .y = 32, .width = 16, .height = 16 };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &multi_options,
        multi_effect
      )) {
    fprintf(stderr, "multi-pass effect render pass failed\n");
    return 1;
  }

  // Plugin image texture input. The external 1x1 texture is green while the
  // window source is red, so the output proves the sampler was uploaded.
  auto image_texture = std::make_shared<ura::EffectTexture>();
  image_texture->width = 1;
  image_texture->height = 1;
  image_texture->pixels = { 0, 255, 0, 255 };
  auto texture_pipeline = std::make_shared<ura::EffectPipeline>();
  texture_pipeline->handle = ura::EffectHandle { .id = 4, .generation = 1 };
  texture_pipeline->name = "test-texture";
  texture_pipeline->passes.push_back(
    ura::EffectPass {
      .id = {},
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D noise;
void main() { gl_FragColor = texture2D(noise, fragTexCoord); }
)",
      .inputs = {
        ura::EffectPassInput {
          .name = "noise",
          .source = "texture",
          .texture = image_texture,
        },
      },
      .uniforms = {},
    }
  );
  ura::EffectInstance texture_instance;
  texture_instance.pipeline = texture_pipeline;
  auto texture_effect = texture_instance;
  auto external_options = texture_options;
  external_options.dst_box = { .x = 28, .y = 8, .width = 16, .height = 16 };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &external_options,
        texture_effect
      )) {
    fprintf(stderr, "texture effect render pass failed\n");
    return 1;
  }

  // Three-pass graph with named intermediate targets and two samplers.
  auto graph_pipeline = std::make_shared<ura::EffectPipeline>();
  graph_pipeline->handle = ura::EffectHandle { .id = 3, .generation = 1 };
  graph_pipeline->name = "test-graph";
  graph_pipeline->passes = {
    ura::EffectPass {
      .id = "copy",
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
void main() { gl_FragColor = texture2D(texture0, fragTexCoord); }
)",
      .inputs = {},
      .uniforms = {},
    },
    ura::EffectPass {
      .id = "green",
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
void main() { gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); }
)",
      .inputs = {},
      .uniforms = {},
    },
    ura::EffectPass {
      .id = {},
      .fragment_shader = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D input0;
uniform sampler2D input1;
void main() {
  gl_FragColor = texture2D(input0, fragTexCoord)
    + texture2D(input1, fragTexCoord);
}
)",
      .inputs = {
        ura::EffectPassInput {
          .name = "input0",
          .source = "copy",
          .texture = {},
        },
        ura::EffectPassInput {
          .name = "input1",
          .source = "green",
          .texture = {},
        },
      },
      .uniforms = {},
    },
  };
  ura::EffectInstance graph_instance;
  graph_instance.pipeline = graph_pipeline;
  auto graph_effect = graph_instance;
  auto graph_options = texture_options;
  graph_options.dst_box = { .x = 8, .y = 32, .width = 16, .height = 16 };
  if (!ura::render::raylib::render_pass_add_texture_effect(
        pass,
        &graph_options,
        graph_effect
      )) {
    fprintf(stderr, "effect graph render pass failed\n");
    return 1;
  }

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
  uint32_t custom = pixel_at(target, 52, 12);
  if (custom != 0xFF00FF00u) {
    fprintf(stderr, "unexpected custom effect pixel: 0x%08x\n", custom);
    return 1;
  }

  uint32_t multi = pixel_at(target, 52, 36);
  if (multi != 0xFF0000FFu) {
    fprintf(stderr, "unexpected multi-pass pixel: 0x%08x\n", multi);
    return 1;
  }

  uint32_t external = pixel_at(target, 32, 12);
  if (external != 0xFF00FF00u) {
    fprintf(stderr, "unexpected external texture pixel: 0x%08x\n", external);
    return 1;
  }

  uint32_t graph = pixel_at(target, 12, 36);
  if (graph != 0xFFFFFF00u) {
    fprintf(stderr, "unexpected graph effect pixel: 0x%08x\n", graph);
    return 1;
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

  // Rotation-only composition: the window is rendered into an offscreen
  // target and composited as a rotated quad.
  TestBuffer* rotation_target = create_test_buffer(32, 32, 0);
  if (!rotation_target) {
    fprintf(stderr, "failed to create rotation target\n");
    return 1;
  }
  wlr_render_pass* rotation_pass =
    wlr_renderer_begin_buffer_pass(renderer, &rotation_target->base, nullptr);
  if (!rotation_pass) {
    fprintf(stderr, "failed to begin rotation render pass\n");
    return 1;
  }
  wlr_render_rect_options rotation_background {};
  rotation_background.box = { .x = 0, .y = 0, .width = 32, .height = 32 };
  rotation_background.color = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
  rotation_background.blend_mode = WLR_RENDER_BLEND_MODE_NONE;
  wlr_render_pass_add_rect(rotation_pass, &rotation_background);

  auto rotation_options = texture_options;
  rotation_options.dst_box = { .x = 8, .y = 8, .width = 16, .height = 16 };
  rotation_options.wait_timeline = nullptr;
  rotation_options.wait_point = 0;
  ura::render::raylib::EffectGroupItem rotation_item {
    .texture = &rotation_options,
  };
  ura::EffectInstance rotation_effect;
  if (!ura::render::raylib::render_pass_add_group_effect(
        rotation_pass,
        &rotation_item,
        1,
        rotation_effect,
        1.0f,
        0.78539816339f,
        16.0,
        16.0
      )) {
    fprintf(stderr, "rotation render pass failed\n");
    return 1;
  }
  if (!wlr_render_pass_submit(rotation_pass)) {
    fprintf(stderr, "rotation render pass submit failed\n");
    return 1;
  }
  if (
    pixel_at(rotation_target, 16, 16) != 0xFFFF0000u
    || pixel_at(rotation_target, 8, 8) != 0xFFFFFFFFu
  ) {
    fprintf(
      stderr,
      "unexpected rotation pixels: center=0x%08x corner=0x%08x\n",
      pixel_at(rotation_target, 16, 16),
      pixel_at(rotation_target, 8, 8)
    );
    return 1;
  }
  wlr_buffer_drop(&rotation_target->base);

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
