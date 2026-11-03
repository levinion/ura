#include "ura/render/raylib/raylib_renderer.hpp"
#include "ura/core/log.hpp"
#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>

namespace ura::render::raylib {

static int renderer_timer_get_duration_ns(wlr_render_timer* timer);
static void renderer_timer_destroy(wlr_render_timer* timer);

static const wlr_render_timer_impl render_timer_impl = {
  .get_duration_ns = renderer_timer_get_duration_ns,
  .destroy = renderer_timer_destroy,
};

static const wlr_drm_format_set*
renderer_get_texture_formats(wlr_renderer* wlr_renderer, uint32_t buffer_caps) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);
  if (buffer_caps & WLR_BUFFER_CAP_DMABUF)
    return &renderer->egl.texture_formats;
  if (buffer_caps & (WLR_BUFFER_CAP_DATA_PTR | WLR_BUFFER_CAP_SHM))
    return &renderer->egl.shm_formats;
  return nullptr;
}

static const wlr_drm_format_set*
renderer_get_render_formats(wlr_renderer* wlr_renderer) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);
  if (renderer->egl.render_formats.len > 0)
    return &renderer->egl.render_formats;
  return &renderer->egl.shm_formats;
}

static int renderer_get_drm_fd(wlr_renderer* wlr_renderer) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);
  return renderer->egl.drm_fd;
}

static wlr_texture*
renderer_texture_from_buffer(wlr_renderer* wlr_renderer, wlr_buffer* buffer) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);
  Texture* texture = texture_from_buffer(renderer, buffer);
  return texture ? &texture->base : nullptr;
}

static wlr_render_pass* renderer_begin_buffer_pass(
  wlr_renderer* wlr_renderer,
  wlr_buffer* buffer,
  const wlr_buffer_pass_options* options
) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);
  RenderPass* pass = render_pass_begin(renderer, buffer, options);
  return pass ? &pass->base : nullptr;
}

static wlr_render_timer* renderer_timer_create(wlr_renderer* wlr_renderer) {
  (void)wlr_renderer;
  auto* timer = static_cast<RenderTimer*>(calloc(1, sizeof(RenderTimer)));
  if (!timer)
    return nullptr;
  timer->base.impl = &render_timer_impl;
  timer->start = {};
  timer->end = {};
  timer->started = false;
  return &timer->base;
}

static int renderer_timer_get_duration_ns(wlr_render_timer* wlr_timer) {
  RenderTimer* timer = wl_container_of(wlr_timer, timer, base);
  if (timer->end.tv_sec == 0 && timer->end.tv_nsec == 0)
    return -1;
  time_t sec = timer->end.tv_sec - timer->start.tv_sec;
  long nsec = timer->end.tv_nsec - timer->start.tv_nsec;
  return static_cast<int>(sec * 1000000000L + nsec);
}

static void renderer_timer_destroy(wlr_render_timer* wlr_timer) {
  RenderTimer* timer = wl_container_of(wlr_timer, timer, base);
  free(timer);
}

static void renderer_destroy(wlr_renderer* wlr_renderer) {
  Renderer* renderer = wl_container_of(wlr_renderer, renderer, base);

  EglSavedContext saved {};
  bool current = egl_make_current(&renderer->egl, &saved);

  Texture* texture;
  Texture* texture_tmp;
  wl_list_for_each_safe(texture, texture_tmp, &renderer->textures, link) {
    texture_destroy(texture);
  }

  Buffer* buffer;
  Buffer* buffer_tmp;
  wl_list_for_each_safe(buffer, buffer_tmp, &renderer->buffers, link) {
    buffer_finish(buffer);
  }

  if (current) {
    effect_targets_finish(renderer);
    shader_finish(renderer);
    if (renderer->rlgl_ready)
      rlglClose();
  }
  renderer->rlgl_ready = false;

  renderer_finish_surface_formats(renderer);

  if (current)
    egl_restore_context(&saved);

  egl_context_finish(&renderer->egl);
  free(renderer);
}

static const wlr_renderer_impl renderer_impl = {
  .get_texture_formats = renderer_get_texture_formats,
  .get_render_formats = renderer_get_render_formats,
  .destroy = renderer_destroy,
  .get_drm_fd = renderer_get_drm_fd,
  .texture_from_buffer = renderer_texture_from_buffer,
  .begin_buffer_pass = renderer_begin_buffer_pass,
  .render_timer_create = renderer_timer_create,
};

static void cleanup_failed_renderer(Renderer* renderer) {
  if (!renderer)
    return;

  EglSavedContext saved {};
  bool current = false;
  if (
    renderer->egl.display != EGL_NO_DISPLAY
    && renderer->egl.context != EGL_NO_CONTEXT
  ) {
    current = egl_make_current(&renderer->egl, &saved);
  }

  if (current) {
    effect_targets_finish(renderer);
    if (renderer->rlgl_ready) {
      shader_finish(renderer);
      rlglClose();
    }
  }
  renderer->rlgl_ready = false;

  if (current)
    egl_restore_context(&saved);

  renderer_finish_surface_formats(renderer);
  egl_context_finish(&renderer->egl);
  free(renderer);
}

} // namespace ura::render::raylib

extern "C" struct wlr_renderer*
ura_raylib_renderer_create(struct wlr_backend* backend) {
  using namespace ura::render::raylib;

  if (!backend)
    return nullptr;

  auto* renderer = static_cast<Renderer*>(calloc(1, sizeof(Renderer)));
  if (!renderer)
    return nullptr;

  renderer->drm_fd = -1;
  renderer->white_texture = 0;
  wl_list_init(&renderer->buffers);
  wl_list_init(&renderer->textures);

  if (!egl_context_init(&renderer->egl, backend)) {
    ura::log::error("Failed to initialize EGL for raylib renderer");
    cleanup_failed_renderer(renderer);
    return nullptr;
  }

  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved)) {
    cleanup_failed_renderer(renderer);
    return nullptr;
  }

  rlLoadExtensions(reinterpret_cast<void*>(eglGetProcAddress));
  rlglInit(1, 1);
  if (rlGetShaderIdDefault() == 0 || rlGetTextureIdDefault() == 0) {
    ura::log::error("Failed to initialize rlgl");
    egl_restore_context(&saved);
    renderer->rlgl_ready = true; // Make cleanup call rlglClose().
    cleanup_failed_renderer(renderer);
    return nullptr;
  }
  renderer->rlgl_ready = true;
  renderer->white_texture = rlGetTextureIdDefault();

  const char* gl_renderer =
    reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* gl_version =
    reinterpret_cast<const char*>(glGetString(GL_VERSION));
  ura::log::info(
    "Raylib renderer GL: {} ({})",
    gl_renderer ? gl_renderer : "unknown",
    gl_version ? gl_version : "unknown"
  );

  if (!shader_init(renderer)) {
    egl_restore_context(&saved);
    cleanup_failed_renderer(renderer);
    return nullptr;
  }

  if (!renderer_init_surface_formats(renderer)) {
    ura::log::error("Raylib renderer has no usable surface formats");
    egl_restore_context(&saved);
    cleanup_failed_renderer(renderer);
    return nullptr;
  }

  egl_restore_context(&saved);

  uint32_t caps = WLR_BUFFER_CAP_DATA_PTR | WLR_BUFFER_CAP_SHM;
  if (renderer->egl.has_dmabuf_import)
    caps |= WLR_BUFFER_CAP_DMABUF;

  wlr_renderer_init(&renderer->base, &renderer_impl, caps);
  renderer->base.features.input_color_transform = false;
  renderer->base.features.output_color_transform = false;
  renderer->base.features.timeline = renderer->egl.has_native_fence
    && renderer->egl.has_fence_sync && renderer->egl.has_wait_sync;
  renderer->drm_fd = renderer->egl.drm_fd;

  ura::log::info(
    "Created raylib renderer (GLES2 via rlgl, drm_fd={}, dmabuf={}, "
    "modifiers={}, timeline={})",
    renderer->drm_fd,
    renderer->egl.has_dmabuf_import ? "yes" : "no",
    renderer->egl.has_dmabuf_modifiers ? "yes" : "no",
    renderer->base.features.timeline ? "yes" : "no"
  );

  return &renderer->base;
}
