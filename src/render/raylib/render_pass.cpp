#include "ura/render/raylib/renderer.hpp"
#include "ura/core/log.hpp"
#include "ura/render/effect.hpp"
#include "ura/render/raylib/effect_renderer.hpp"

#include <raymath.h>

#include <unistd.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {
#include <wlr/util/box.h>
#include <wlr/util/transform.h>
}

namespace ura::render::raylib {

static bool render_pass_submit_cb(wlr_render_pass* pass);
static void render_pass_add_texture_cb(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options
);
static void render_pass_add_rect_cb(
  wlr_render_pass* pass,
  const wlr_render_rect_options* options
);

static const wlr_render_pass_impl render_pass_impl = {
  .submit = render_pass_submit_cb,
  .add_texture = render_pass_add_texture_cb,
  .add_rect = render_pass_add_rect_cb,
};

// raymath stores matrices in row-major order, but MatrixMultiply(A, B)
// returns B * A for that layout. Callers below therefore put the new factor
// on the left to append it to the current transform.
static Matrix transform_matrix(enum wl_output_transform transform) {
  switch (transform) {
    case WL_OUTPUT_TRANSFORM_NORMAL:
      return MatrixIdentity();
    case WL_OUTPUT_TRANSFORM_90:
      return MatrixRotateZ(90.0f * DEG2RAD);
    case WL_OUTPUT_TRANSFORM_180:
      return MatrixRotateZ(180.0f * DEG2RAD);
    case WL_OUTPUT_TRANSFORM_270:
      return MatrixRotateZ(270.0f * DEG2RAD);
    case WL_OUTPUT_TRANSFORM_FLIPPED:
      return MatrixScale(-1.0f, 1.0f, 1.0f);
    case WL_OUTPUT_TRANSFORM_FLIPPED_90:
      return MatrixMultiply(
        MatrixRotateZ(90.0f * DEG2RAD),
        MatrixScale(-1.0f, 1.0f, 1.0f)
      );
    case WL_OUTPUT_TRANSFORM_FLIPPED_180:
      return MatrixMultiply(
        MatrixRotateZ(180.0f * DEG2RAD),
        MatrixScale(-1.0f, 1.0f, 1.0f)
      );
    case WL_OUTPUT_TRANSFORM_FLIPPED_270:
      return MatrixMultiply(
        MatrixRotateZ(270.0f * DEG2RAD),
        MatrixScale(-1.0f, 1.0f, 1.0f)
      );
  }
  return MatrixIdentity();
}

static Matrix set_tex_matrix(
  const wlr_render_texture_options* options,
  const Texture* texture
) {
  wlr_fbox src {};
  wlr_render_texture_options_get_src_box(options, &src);
  src.x /= texture->base.width;
  src.y /= texture->base.height;
  src.width /= texture->base.width;
  src.height /= texture->base.height;

  auto transform = options->transform;
  if (transform & WL_OUTPUT_TRANSFORM_90)
    transform = wlr_output_transform_invert(transform);

  Matrix matrix = MatrixIdentity();
  matrix = MatrixMultiply(
    MatrixTranslate(static_cast<float>(src.x), static_cast<float>(src.y), 0),
    matrix
  );
  matrix = MatrixMultiply(
    MatrixScale(
      static_cast<float>(src.width),
      static_cast<float>(src.height),
      1
    ),
    matrix
  );
  matrix = MatrixMultiply(MatrixTranslate(0.5f, 0.5f, 0), matrix);
  matrix = MatrixMultiply(transform_matrix(transform), matrix);
  matrix = MatrixMultiply(MatrixTranslate(-0.5f, -0.5f, 0), matrix);
  return matrix;
}

static void damage_add_box(RenderPass* pass, const wlr_box* box) {
  wlr_box clipped {};
  wlr_box bounds = {
    .x = 0,
    .y = 0,
    .width = pass->width,
    .height = pass->height,
  };
  if (!wlr_box_intersection(&clipped, box, &bounds))
    return;
  if (clipped.width <= 0 || clipped.height <= 0)
    return;
  pixman_region32_union_rect(
    &pass->damage,
    &pass->damage,
    clipped.x,
    clipped.y,
    clipped.width,
    clipped.height
  );
}

static void reset_vertex_attributes() {
  rlDisableVertexArray();
  for (GLuint i = 0; i < 8; i++) glDisableVertexAttribArray(i);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void flush_batch(RenderPass* pass) {
  // Rendering uses direct GL calls. rlgl is only used for optional effect
  // experiments; keep its batch from touching the direct GL state here.
  (void)pass;
}

static void setup_blend(RenderPass* pass, bool none) {
  flush_batch(pass);
  if (none) {
    glDisable(GL_BLEND);
  } else {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }
}

static bool scissor_rect(RenderPass* pass, const pixman_box32_t* rect) {
  int x = rect->x1;
  int y = rect->y1;
  int width = rect->x2 - rect->x1;
  int height = rect->y2 - rect->y1;
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (x + width > pass->width)
    width = pass->width - x;
  if (y + height > pass->height)
    height = pass->height - y;
  if (width <= 0 || height <= 0)
    return false;

  glEnable(GL_SCISSOR_TEST);
  glScissor(x, y, width, height);
  return true;
}

static void draw_rect(
  RenderPass* pass,
  const pixman_box32_t* rect,
  const wlr_render_color* color
) {
  Renderer* renderer = pass->renderer;
  if (!renderer->rect.valid)
    return;

  flush_batch(pass);
  if (!scissor_rect(pass, rect))
    return;

  float x1 = static_cast<float>(rect->x1);
  float y1 = static_cast<float>(rect->y1);
  float x2 = static_cast<float>(rect->x2);
  float y2 = static_cast<float>(rect->y2);
  const float positions[] = {
    x1, y1, x2, y1, x2, y2, x1, y1, x2, y2, x1, y2,
  };

  reset_vertex_attributes();
  glUseProgram(renderer->rect.id);
  shader_set_projection(renderer->rect.mvp, pass->width, pass->height);
  glUniform4f(renderer->rect.color, color->r, color->g, color->b, color->a);
  glEnableVertexAttribArray(renderer->rect.pos_attrib);
  glVertexAttribPointer(
    renderer->rect.pos_attrib,
    2,
    GL_FLOAT,
    GL_FALSE,
    0,
    positions
  );
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(renderer->rect.pos_attrib);
  glUseProgram(0);

  glDisable(GL_SCISSOR_TEST);
}

static bool create_shm_target(RenderPass* pass, const GlFormat* format) {
  while (glGetError() != GL_NO_ERROR) {}
  auto bpp = format->bytes_per_pixel();
  if (bpp == 0)
    return false;

  glGenTextures(1, &pass->shm_texture);
  glBindTexture(GL_TEXTURE_2D, pass->shm_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    format->internal_format ? format->internal_format : format->format,
    pass->width,
    pass->height,
    0,
    format->format,
    format->type,
    nullptr
  );

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  auto bytes = static_cast<const uint8_t*>(pass->shm_data);
  bool swizzle = format->needs_swizzle();
  uint8_t* row = nullptr;
  if (swizzle) {
    row = static_cast<uint8_t*>(malloc(static_cast<size_t>(pass->width) * 4));
    if (!row) {
      glDeleteTextures(1, &pass->shm_texture);
      pass->shm_texture = 0;
      return false;
    }
  }
  for (int y = 0; y < pass->height; y++) {
    const void* src = bytes + static_cast<size_t>(y) * pass->shm_stride;
    if (swizzle) {
      format->swizzle_row(static_cast<const uint8_t*>(src), row, pass->width);
      src = row;
    }
    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      y,
      pass->width,
      1,
      format->format,
      format->type,
      src
    );
  }
  free(row);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &pass->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    pass->shm_texture,
    0
  );
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    log::error("SHM framebuffer is incomplete: 0x{:x}", status);
    glDeleteFramebuffers(1, &pass->fbo);
    glDeleteTextures(1, &pass->shm_texture);
    pass->fbo = 0;
    pass->shm_texture = 0;
    return false;
  }
  return true;
}

static bool convert_readback_row(
  const uint8_t* rgba,
  uint8_t* dst,
  int width,
  uint32_t format
) {
  switch (format) {
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_XBGR8888:
      memcpy(dst, rgba, static_cast<size_t>(width) * 4);
      return true;
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XRGB8888:
      for (int x = 0; x < width; x++) {
        dst[x * 4 + 0] = rgba[x * 4 + 2];
        dst[x * 4 + 1] = rgba[x * 4 + 1];
        dst[x * 4 + 2] = rgba[x * 4 + 0];
        dst[x * 4 + 3] = rgba[x * 4 + 3];
      }
      return true;
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_BGRX8888:
      for (int x = 0; x < width; x++) {
        dst[x * 4 + 0] = rgba[x * 4 + 3];
        dst[x * 4 + 1] = rgba[x * 4 + 0];
        dst[x * 4 + 2] = rgba[x * 4 + 1];
        dst[x * 4 + 3] = rgba[x * 4 + 2];
      }
      return true;
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_RGBX8888:
      for (int x = 0; x < width; x++) {
        dst[x * 4 + 0] = rgba[x * 4 + 3];
        dst[x * 4 + 1] = rgba[x * 4 + 2];
        dst[x * 4 + 2] = rgba[x * 4 + 1];
        dst[x * 4 + 3] = rgba[x * 4 + 0];
      }
      return true;
    case DRM_FORMAT_RGB565:
      for (int x = 0; x < width; x++) {
        uint16_t r = rgba[x * 4 + 0] >> 3;
        uint16_t g = rgba[x * 4 + 1] >> 2;
        uint16_t b = rgba[x * 4 + 2] >> 3;
        uint16_t value = static_cast<uint16_t>((r << 11) | (g << 5) | b);
        memcpy(dst + x * 2, &value, sizeof(value));
      }
      return true;
    case DRM_FORMAT_ABGR2101010:
    case DRM_FORMAT_XBGR2101010:
      for (int x = 0; x < width; x++) {
        uint32_t r = static_cast<uint32_t>(rgba[x * 4 + 0]) << 2;
        uint32_t g = static_cast<uint32_t>(rgba[x * 4 + 1]) << 2;
        uint32_t b = static_cast<uint32_t>(rgba[x * 4 + 2]) << 2;
        uint32_t a = format == DRM_FORMAT_ABGR2101010
          ? static_cast<uint32_t>(rgba[x * 4 + 3]) << 2
          : 0x3u;
        uint32_t value = (a << 30) | (b << 20) | (g << 10) | r;
        memcpy(dst + x * 4, &value, sizeof(value));
      }
      return true;
    default:
      return false;
  }
}

static bool readback_shm(RenderPass* pass) {
  if (!pass->shm_access || !pass->shm_writable)
    return false;

  auto* format = gl_format_from_drm(pass->shm_format);
  if (!format)
    return false;
  auto bpp = format->bytes_per_pixel();
  if (bpp == 0)
    return false;
  if (pass->shm_stride < static_cast<size_t>(pass->width) * bpp)
    return false;

  glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
  glFinish();
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  auto row =
    static_cast<uint8_t*>(malloc(static_cast<size_t>(pass->width) * 4));
  if (!row)
    return false;

  auto dst = static_cast<uint8_t*>(pass->shm_data);
  bool ok = true;
  for (int y = 0; y < pass->height; y++) {
    glReadPixels(0, y, pass->width, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
    if (!convert_readback_row(
          row,
          dst + static_cast<size_t>(y) * pass->shm_stride,
          pass->width,
          pass->shm_format
        )) {
      ok = false;
      break;
    }
  }

  free(row);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return ok;
}

static bool signal_timeline(RenderPass* pass) {
  Renderer* renderer = pass->renderer;
  EglContext* egl = &renderer->egl;
  if (!egl->has_native_fence || !egl->create_sync || !egl->dup_native_fence_fd)
    return false;

  const EGLint attrs[] = {
    EGL_SYNC_NATIVE_FENCE_FD_ANDROID,
    -1,
    EGL_NONE,
  };
  EGLSyncKHR sync =
    egl->create_sync(egl->display, EGL_SYNC_NATIVE_FENCE_ANDROID, attrs);
  if (sync == EGL_NO_SYNC_KHR) {
    log::error("Failed to create EGL native fence sync");
    return false;
  }

  glFlush();
  int fd = egl->dup_native_fence_fd(egl->display, sync);
  egl->destroy_sync(egl->display, sync);
  if (fd < 0) {
    log::error("Failed to export EGL native fence");
    return false;
  }

  bool ok = wlr_drm_syncobj_timeline_import_sync_file(
    pass->signal_timeline,
    pass->signal_point,
    fd
  );
  close(fd);
  return ok;
}

RenderPass* render_pass_begin(
  Renderer* renderer,
  wlr_buffer* buffer,
  const wlr_buffer_pass_options* options
) {
  if (!renderer || !buffer)
    return nullptr;

  auto* pass = static_cast<RenderPass*>(calloc(1, sizeof(RenderPass)));
  if (!pass)
    return nullptr;

  wlr_render_pass_init(&pass->base, &render_pass_impl);
  pass->renderer = renderer;
  pass->buffer = wlr_buffer_lock(buffer);
  pass->width = buffer->width;
  pass->height = buffer->height;
  pass->shm_format = DRM_FORMAT_INVALID;
  pixman_region32_init(&pass->damage);

  if (options) {
    pass->signal_timeline = options->signal_timeline
      ? wlr_drm_syncobj_timeline_ref(options->signal_timeline)
      : nullptr;
    pass->signal_point = options->signal_point;
    if (options->timer)
      pass->timer = wl_container_of(options->timer, pass->timer, base);
  }

  if (!egl_make_current(&renderer->egl, &pass->saved)) {
    if (pass->signal_timeline)
      wlr_drm_syncobj_timeline_unref(pass->signal_timeline);
    wlr_buffer_unlock(pass->buffer);
    pixman_region32_fini(&pass->damage);
    free(pass);
    return nullptr;
  }

  wlr_dmabuf_attributes attributes {};
  if (wlr_buffer_get_dmabuf(buffer, &attributes)) {
    pass->target = buffer_get(renderer, buffer);
    if (!pass->target || !buffer_ensure_render_target(pass->target)) {
      egl_restore_context(&pass->saved);
      if (pass->signal_timeline)
        wlr_drm_syncobj_timeline_unref(pass->signal_timeline);
      wlr_buffer_unlock(pass->buffer);
      pixman_region32_fini(&pass->damage);
      free(pass);
      return nullptr;
    }
    pass->fbo = pass->target->fbo;
  } else {
    void* data = nullptr;
    uint32_t format = DRM_FORMAT_INVALID;
    size_t stride = 0;
    uint32_t access =
      WLR_BUFFER_DATA_PTR_ACCESS_READ | WLR_BUFFER_DATA_PTR_ACCESS_WRITE;
    if (!wlr_buffer_begin_data_ptr_access(
          buffer,
          access,
          &data,
          &format,
          &stride
        )) {
      access = WLR_BUFFER_DATA_PTR_ACCESS_READ;
      if (!wlr_buffer_begin_data_ptr_access(
            buffer,
            access,
            &data,
            &format,
            &stride
          )) {
        egl_restore_context(&pass->saved);
        if (pass->signal_timeline)
          wlr_drm_syncobj_timeline_unref(pass->signal_timeline);
        wlr_buffer_unlock(pass->buffer);
        pixman_region32_fini(&pass->damage);
        free(pass);
        return nullptr;
      }
      log::info("SHM render target is read-only");
    }

    pass->shm_access = true;
    pass->shm_writable = access & WLR_BUFFER_DATA_PTR_ACCESS_WRITE;
    pass->shm_data = data;
    pass->shm_format = format;
    pass->shm_stride = stride;

    const GlFormat* gl_format = gl_format_from_drm(format);
    if (!gl_format || !create_shm_target(pass, gl_format)) {
      wlr_buffer_end_data_ptr_access(buffer);
      egl_restore_context(&pass->saved);
      if (pass->signal_timeline)
        wlr_drm_syncobj_timeline_unref(pass->signal_timeline);
      wlr_buffer_unlock(pass->buffer);
      pixman_region32_fini(&pass->damage);
      free(pass);
      return nullptr;
    }
  }

  if (pass->timer) {
    clock_gettime(CLOCK_MONOTONIC, &pass->timer->start);
    pass->timer->started = true;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
  glViewport(0, 0, pass->width, pass->height);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  if (renderer->rlgl_ready) {
    pass->rlgl_active = true;
    rlSetFramebufferWidth(pass->width);
    rlSetFramebufferHeight(pass->height);
    rlViewport(0, 0, pass->width, pass->height);
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(
      0.0,
      static_cast<double>(pass->width),
      0.0,
      static_cast<double>(pass->height),
      -1.0,
      1.0
    );
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
  }

  return pass;
}

static void destroy_pass(RenderPass* pass) {
  if (!pass)
    return;

  if (pass->shm_texture) {
    glDeleteFramebuffers(1, &pass->fbo);
    glDeleteTextures(1, &pass->shm_texture);
  }

  if (pass->shm_access)
    wlr_buffer_end_data_ptr_access(pass->buffer);

  if (pass->signal_timeline)
    wlr_drm_syncobj_timeline_unref(pass->signal_timeline);

  egl_restore_context(&pass->saved);
  wlr_buffer_unlock(pass->buffer);
  pixman_region32_fini(&pass->damage);
  free(pass);
}

bool render_pass_submit(RenderPass* pass) {
  if (!pass)
    return false;

  glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
  effects_apply(pass->renderer, pass);
  flush_batch(pass);

  bool ok = true;
  if (pass->shm_texture) {
    if (!readback_shm(pass)) {
      log::error("Failed to read back SHM render target");
      ok = false;
    }
  }

  if (pass->signal_timeline) {
    if (!signal_timeline(pass)) {
      log::error("Failed to signal render timeline");
      ok = false;
    }
  } else if (!pass->shm_texture) {
    // No explicit synchronization was supplied by the caller. Keep the
    // legacy GPU-completion barrier only for this fallback path.
    glFinish();
  }

  if (pass->timer && pass->timer->started) {
    clock_gettime(CLOCK_MONOTONIC, &pass->timer->end);
    pass->timer->started = false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  destroy_pass(pass);
  return ok;
}

void render_pass_add_rect(
  RenderPass* pass,
  const wlr_render_rect_options* options
) {
  if (!pass || !options)
    return;

  wlr_box box {};
  wlr_render_rect_options_get_box(options, pass->buffer, &box);
  if (box.width <= 0 || box.height <= 0)
    return;

  pixman_region32_t region;
  pixman_region32_init_rect(&region, box.x, box.y, box.width, box.height);
  if (options->clip)
    pixman_region32_intersect(&region, &region, options->clip);

  bool blend_none = options->color.a == 1.0f
    || options->blend_mode == WLR_RENDER_BLEND_MODE_NONE;
  setup_blend(pass, blend_none);

  int rects_len = 0;
  const pixman_box32_t* rects = pixman_region32_rectangles(&region, &rects_len);
  for (int i = 0; i < rects_len; i++)
    draw_rect(pass, &rects[i], &options->color);

  pixman_region32_fini(&region);
  damage_add_box(pass, &box);
}

static bool wait_for_timeline(
  RenderPass* pass,
  wlr_drm_syncobj_timeline* timeline,
  uint64_t point
) {
  if (!timeline)
    return true;

  EglContext* egl = &pass->renderer->egl;
  if (!egl->has_native_fence || !egl->create_sync || !egl->destroy_sync)
    return false;

  int fd = wlr_drm_syncobj_timeline_export_sync_file(timeline, point);
  if (fd < 0) {
    log::error("Failed to export render wait timeline");
    return false;
  }

  const EGLint attrs[] = {
    EGL_SYNC_NATIVE_FENCE_FD_ANDROID,
    fd,
    EGL_NONE,
  };
  EGLSyncKHR sync =
    egl->create_sync(egl->display, EGL_SYNC_NATIVE_FENCE_ANDROID, attrs);
  if (sync == EGL_NO_SYNC_KHR) {
    close(fd);
    log::error("Failed to create EGL wait fence");
    return false;
  }

  bool ok = true;
  if (egl->wait_sync) {
    ok = egl->wait_sync(egl->display, sync, 0);
  } else if (egl->client_wait_sync) {
    ok = egl->client_wait_sync(egl->display, sync, 0, EGL_FOREVER_KHR)
      == EGL_CONDITION_SATISFIED_KHR;
  } else {
    ok = false;
  }
  egl->destroy_sync(egl->display, sync);
  return ok;
}

static void effect_target_destroy(EffectTarget& target) {
  if (target.fbo) {
    glDeleteFramebuffers(1, &target.fbo);
    target.fbo = 0;
  }
  if (target.texture) {
    glDeleteTextures(1, &target.texture);
    target.texture = 0;
  }
  target.width = 0;
  target.height = 0;
}

void effect_targets_finish(Renderer* renderer) {
  if (!renderer || !renderer->effect_targets)
    return;
  for (auto& target : *renderer->effect_targets) effect_target_destroy(target);
  delete renderer->effect_targets;
  renderer->effect_targets = nullptr;
}

static EffectTarget*
effect_target_ensure(Renderer* renderer, int index, int width, int height) {
  if (!renderer->effect_targets)
    renderer->effect_targets = new std::vector<EffectTarget>;
  auto& targets = *renderer->effect_targets;
  if (targets.size() <= static_cast<std::size_t>(index))
    targets.resize(static_cast<std::size_t>(index) + 1);
  auto& target = targets[index];
  if (target.texture && target.width == width && target.height == height)
    return &target;

  effect_target_destroy(target);

  while (glGetError() != GL_NO_ERROR) {}
  glGenTextures(1, &target.texture);
  glBindTexture(GL_TEXTURE_2D, target.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    width,
    height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    nullptr
  );
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenFramebuffers(1, &target.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    target.texture,
    0
  );
  auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    log::error("EffectInstance framebuffer is incomplete: 0x{:x}", status);
    effect_target_destroy(target);
    return nullptr;
  }

  target.width = width;
  target.height = height;
  return &target;
}

static void effect_pass_begin(int width, int height, GLuint fbo) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, width, height);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

static void draw_effect_copy(
  Renderer* renderer,
  Texture* source,
  const wlr_render_texture_options* options,
  int target_width,
  int target_height,
  const wlr_box& dst,
  float alpha
) {
  auto matrix = set_tex_matrix(options, source);
  auto uv0 = Vector2Transform({ 0.0f, 0.0f }, matrix);
  auto uv1 = Vector2Transform({ 1.0f, 0.0f }, matrix);
  auto uv2 = Vector2Transform({ 1.0f, 1.0f }, matrix);
  auto uv3 = Vector2Transform({ 0.0f, 1.0f }, matrix);

  float x1 = static_cast<float>(dst.x);
  float y1 = static_cast<float>(dst.y);
  float x2 = static_cast<float>(dst.x + dst.width);
  float y2 = static_cast<float>(dst.y + dst.height);
  const float vertices[] = {
    x1, y1, uv0.x, uv0.y, x2, y1, uv1.x, uv1.y, x2, y2, uv2.x, uv2.y,
    x1, y1, uv0.x, uv0.y, x2, y2, uv2.x, uv2.y, x1, y2, uv3.x, uv3.y,
  };

  auto& shader = renderer->effect_copy;
  reset_vertex_attributes();
  glUseProgram(shader.id);
  shader_set_projection(shader.mvp, target_width, target_height);
  glUniform1i(shader.texture, 0);
  glUniform4f(shader.color, 1.0f, 1.0f, 1.0f, alpha);
  glUniform4f(shader.tint, 1.0f, 1.0f, 1.0f, 1.0f);
  if (shader.opaque >= 0)
    glUniform1f(shader.opaque, source->has_alpha ? 0.0f : 1.0f);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, source->tex);
  glEnableVertexAttribArray(shader.pos_attrib);
  glVertexAttribPointer(
    shader.pos_attrib,
    4,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    vertices
  );
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(shader.pos_attrib);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

static void draw_effect_rect(
  Renderer* renderer,
  int target_width,
  int target_height,
  const wlr_box& dst,
  const wlr_render_color& color
) {
  auto& shader = renderer->rect;
  if (!shader.valid)
    return;

  float x1 = static_cast<float>(dst.x);
  float y1 = static_cast<float>(dst.y);
  float x2 = static_cast<float>(dst.x + dst.width);
  float y2 = static_cast<float>(dst.y + dst.height);
  const float vertices[] = {
    x1, y1, x2, y1, x2, y2, x1, y1, x2, y2, x1, y2,
  };

  reset_vertex_attributes();
  glUseProgram(shader.id);
  shader_set_projection(shader.mvp, target_width, target_height);
  glUniform4f(shader.color, color.r, color.g, color.b, color.a);
  glEnableVertexAttribArray(shader.pos_attrib);
  glVertexAttribPointer(shader.pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(shader.pos_attrib);
  glUseProgram(0);
}

static void draw_effect_pass(
  Renderer* renderer,
  EffectPassProgram& pass_program,
  const EffectPass& pass,
  const EffectInstance& instance,
  const std::vector<GLuint>& inputs,
  GLuint source,
  GLuint fbo,
  int width,
  int height
) {
  effect_pass_begin(width, height, fbo);

  float w = static_cast<float>(width);
  float h = static_cast<float>(height);
  const float vertices[] = {
    0.0f, 0.0f, 0.0f, 0.0f, w, 0.0f, 1.0f, 0.0f, w,    h, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, w, h,    1.0f, 1.0f, 0.0f, h, 0.0f, 1.0f,
  };

  auto& shader = pass_program.shader;
  reset_vertex_attributes();
  glUseProgram(shader.id);
  shader_set_projection(shader.mvp, width, height);
  if (shader.texture >= 0)
    glUniform1i(shader.texture, 0);

  auto upload = [&](const EffectUniform& uniform) {
    auto it = pass_program.uniforms.find(uniform.name);
    if (it == pass_program.uniforms.end() || it->second < 0)
      return;
    auto location = it->second;
    auto* values = uniform.values.data();
    switch (uniform.type) {
      case EffectUniformType::Float:
        glUniform1f(location, values[0]);
        break;
      case EffectUniformType::Int:
      case EffectUniformType::Bool:
        glUniform1i(location, static_cast<int>(values[0]));
        break;
      case EffectUniformType::Vec2:
        glUniform2f(location, values[0], values[1]);
        break;
      case EffectUniformType::Vec3:
        glUniform3f(location, values[0], values[1], values[2]);
        break;
      case EffectUniformType::Vec4:
        glUniform4f(location, values[0], values[1], values[2], values[3]);
        break;
    }
  };

  for (auto& uniform : instance.uniforms) upload(uniform);
  for (auto& uniform : pass.uniforms) upload(uniform);
  if (pass_program.texel_size >= 0)
    glUniform2f(pass_program.texel_size, 1.0f / w, 1.0f / h);

  if (pass.inputs.empty()) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source);
    auto it = pass_program.samplers.find("texture0");
    if (it != pass_program.samplers.end() && it->second >= 0)
      glUniform1i(it->second, 0);
  } else {
    for (std::size_t i = 0; i < pass.inputs.size() && i < inputs.size(); ++i) {
      glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
      glBindTexture(GL_TEXTURE_2D, inputs[i]);
      auto it = pass_program.samplers.find(pass.inputs[i].name);
      if (it != pass_program.samplers.end() && it->second >= 0)
        glUniform1i(it->second, static_cast<int>(i));
    }
  }
  glEnableVertexAttribArray(shader.pos_attrib);
  glVertexAttribPointer(
    shader.pos_attrib,
    4,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    vertices
  );
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(shader.pos_attrib);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

static void draw_effect_composite(
  RenderPass* pass,
  GLuint texture,
  const wlr_box& dst,
  const wlr_render_texture_options* options,
  float rotation,
  double rotation_center_x,
  double rotation_center_y
) {
  Renderer* renderer = pass->renderer;
  auto& shader = renderer->tex_rgba;
  float alpha = wlr_render_texture_options_get_alpha(options);
  // The offscreen target may contain transparent padding (especially for
  // blur), so alpha == 1 does not imply that opaque overwrite is safe.
  bool blend_none = options->blend_mode == WLR_RENDER_BLEND_MODE_NONE;
  setup_blend(pass, blend_none);

  glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
  glViewport(0, 0, pass->width, pass->height);
  glDisable(GL_SCISSOR_TEST);

  auto corner = [&](double x, double y) {
    if (rotation == 0.f)
      return std::array<double, 2> { x, y };
    auto c = std::cos(rotation);
    auto s = std::sin(rotation);
    auto dx = x - rotation_center_x;
    auto dy = y - rotation_center_y;
    return std::array<double, 2> {
      rotation_center_x + dx * c - dy * s,
      rotation_center_y + dx * s + dy * c,
    };
  };
  auto p0 = corner(dst.x, dst.y);
  auto p1 = corner(dst.x + dst.width, dst.y);
  auto p2 = corner(dst.x + dst.width, dst.y + dst.height);
  auto p3 = corner(dst.x, dst.y + dst.height);
  const float vertices[] = {
    static_cast<float>(p0[0]), static_cast<float>(p0[1]), 0.0f, 0.0f,
    static_cast<float>(p1[0]), static_cast<float>(p1[1]), 1.0f, 0.0f,
    static_cast<float>(p2[0]), static_cast<float>(p2[1]), 1.0f, 1.0f,
    static_cast<float>(p0[0]), static_cast<float>(p0[1]), 0.0f, 0.0f,
    static_cast<float>(p2[0]), static_cast<float>(p2[1]), 1.0f, 1.0f,
    static_cast<float>(p3[0]), static_cast<float>(p3[1]), 0.0f, 1.0f,
  };

  wlr_box damage_box = dst;
  if (rotation != 0.f) {
    auto left = std::min({ p0[0], p1[0], p2[0], p3[0] });
    auto top = std::min({ p0[1], p1[1], p2[1], p3[1] });
    auto right = std::max({ p0[0], p1[0], p2[0], p3[0] });
    auto bottom = std::max({ p0[1], p1[1], p2[1], p3[1] });
    damage_box = {
      static_cast<int>(std::floor(left)),
      static_cast<int>(std::floor(top)),
      static_cast<int>(std::ceil(right) - std::floor(left)),
      static_cast<int>(std::ceil(bottom) - std::floor(top)),
    };
  }

  pixman_region32_t region;
  pixman_region32_init_rect(
    &region,
    damage_box.x,
    damage_box.y,
    damage_box.width,
    damage_box.height
  );
  if (options->clip)
    pixman_region32_intersect(&region, &region, options->clip);

  int rects_len = 0;
  const pixman_box32_t* rects = pixman_region32_rectangles(&region, &rects_len);

  reset_vertex_attributes();
  glUseProgram(shader.id);
  shader_set_projection(shader.mvp, pass->width, pass->height);
  glUniform1i(shader.texture, 0);
  glUniform4f(shader.color, 1.0f, 1.0f, 1.0f, alpha);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glEnableVertexAttribArray(shader.pos_attrib);
  glVertexAttribPointer(
    shader.pos_attrib,
    4,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    vertices
  );

  for (int i = 0; i < rects_len; i++) {
    flush_batch(pass);
    if (!scissor_rect(pass, &rects[i]))
      continue;
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
  }

  glDisableVertexAttribArray(shader.pos_attrib);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  pixman_region32_fini(&region);
  damage_add_box(pass, &damage_box);
}

bool render_pass_add_group_effect(
  wlr_render_pass* base,
  const EffectGroupItem* items,
  std::size_t count,
  const EffectInstance& effect,
  float scale,
  float rotation,
  double rotation_center_x,
  double rotation_center_y
) {
  if (
    !base || base->impl != &render_pass_impl || !items || count == 0
    || (!effect.active() && rotation == 0.f)
  )
    return false;

  RenderPass* pass = wl_container_of(base, pass, base);
  Renderer* renderer = pass->renderer;

  wlr_box union_box {};
  bool has_box = false;
  const pixman_region32_t* clip = nullptr;
  for (std::size_t i = 0; i < count; ++i) {
    wlr_box dst {};
    const wlr_render_texture_options* texture = items[i].texture;
    const wlr_render_rect_options* rect = items[i].rect;
    if (texture) {
      if (!texture->texture)
        return false;
      Texture* source = wl_container_of(texture->texture, source, base);
      if (source->target != GL_TEXTURE_2D)
        return false;
      wlr_render_texture_options_get_dst_box(texture, &dst);
      if (!clip)
        clip = texture->clip;
    } else if (rect) {
      dst = rect->box;
      if (!clip)
        clip = rect->clip;
    } else {
      continue;
    }
    if (dst.width <= 0 || dst.height <= 0)
      continue;

    if (!has_box) {
      union_box = dst;
      has_box = true;
      continue;
    }
    auto right = std::max(union_box.x + union_box.width, dst.x + dst.width);
    auto bottom = std::max(union_box.y + union_box.height, dst.y + dst.height);
    union_box.x = std::min(union_box.x, dst.x);
    union_box.y = std::min(union_box.y, dst.y);
    union_box.width = right - union_box.x;
    union_box.height = bottom - union_box.y;
  }

  if (!has_box)
    return true;

  auto logical_outset = effect.outset;
  auto scale_outset = [&](int value) {
    if (value <= 0)
      return 0;
    return static_cast<int>(
      std::ceil(static_cast<float>(value) * std::max(scale, 0.0f))
    );
  };
  PaintOutset outset {
    .left = scale_outset(logical_outset.left),
    .top = scale_outset(logical_outset.top),
    .right = scale_outset(logical_outset.right),
    .bottom = scale_outset(logical_outset.bottom),
  };
  wlr_box target_box = outset.apply(union_box);
  if (target_box.width <= 0 || target_box.height <= 0)
    return false;

  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  if (
    max_texture_size > 0
    && (target_box.width > max_texture_size || target_box.height > max_texture_size)
  )
    return false;

  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved))
    return false;

  auto restore = [&] {
    glBindFramebuffer(GL_FRAMEBUFFER, pass->fbo);
    glViewport(0, 0, pass->width, pass->height);
    egl_restore_context(&saved);
  };

  auto target_count = effect.active() ? effect.pipeline->passes.size() + 1 : 2;
  target_count = std::max<std::size_t>(target_count, 2);
  for (std::size_t i = 0; i < target_count; ++i) {
    if (!effect_target_ensure(
          renderer,
          static_cast<int>(i),
          target_box.width,
          target_box.height
        )) {
      restore();
      return false;
    }
  }
  auto* target0 = &(*renderer->effect_targets)[0];

  effect_pass_begin(target_box.width, target_box.height, target0->fbo);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  bool drew_any = false;
  for (std::size_t i = 0; i < count; ++i) {
    const wlr_render_texture_options* texture = items[i].texture;
    const wlr_render_rect_options* rect = items[i].rect;
    wlr_box dst {};
    if (texture) {
      if (!texture->texture)
        continue;
      if (!wait_for_timeline(pass, texture->wait_timeline, texture->wait_point))
        continue;
      wlr_render_texture_options_get_dst_box(texture, &dst);
      if (dst.width <= 0 || dst.height <= 0)
        continue;

      Texture* source = wl_container_of(texture->texture, source, base);
      wlr_box relative {
        .x = dst.x - target_box.x,
        .y = dst.y - target_box.y,
        .width = dst.width,
        .height = dst.height,
      };
      draw_effect_copy(
        renderer,
        source,
        texture,
        target_box.width,
        target_box.height,
        relative,
        wlr_render_texture_options_get_alpha(texture)
      );
      drew_any = true;
    } else if (rect) {
      dst = rect->box;
      if (dst.width <= 0 || dst.height <= 0)
        continue;
      wlr_box relative {
        .x = dst.x - target_box.x,
        .y = dst.y - target_box.y,
        .width = dst.width,
        .height = dst.height,
      };
      draw_effect_rect(
        renderer,
        target_box.width,
        target_box.height,
        relative,
        rect->color
      );
      drew_any = true;
    }
  }

  if (!drew_any) {
    glDisable(GL_BLEND);
    restore();
    return true;
  }

  glDisable(GL_BLEND);
  GLuint final_texture = target0->texture;
  if (effect.active()) {
    auto* program = effect_program_get(renderer, effect);
    if (program) {
      GLuint source = target0->texture;
      for (std::size_t i = 0; i < program->passes.size(); ++i) {
        auto& pass_program = program->passes[i];
        auto& pass = effect.pipeline->passes[i];
        auto& destination = (*renderer->effect_targets)[i + 1];

        std::vector<GLuint> inputs;
        inputs.reserve(pass.inputs.size());
        for (auto& input : pass.inputs) {
          if (input.texture) {
            auto texture = effect_texture_get(renderer, *input.texture);
            inputs.push_back(texture ? texture : target0->texture);
            continue;
          }
          if (input.source == "source") {
            inputs.push_back(target0->texture);
            continue;
          }
          if (input.source == "previous") {
            inputs.push_back(source);
            continue;
          }

          GLuint texture = target0->texture;
          for (std::size_t previous = 0; previous < i; ++previous) {
            if (effect.pipeline->passes[previous].id == input.source) {
              texture = (*renderer->effect_targets)[previous + 1].texture;
              break;
            }
          }
          inputs.push_back(texture);
        }

        draw_effect_pass(
          renderer,
          pass_program,
          pass,
          effect,
          inputs,
          source,
          destination.fbo,
          target_box.width,
          target_box.height
        );
        source = destination.texture;
      }
      final_texture = source;
    }
  }

  wlr_render_texture_options composite {};
  composite.dst_box = target_box;
  composite.clip = clip;
  composite.blend_mode = WLR_RENDER_BLEND_MODE_PREMULTIPLIED;
  draw_effect_composite(
    pass,
    final_texture,
    target_box,
    &composite,
    rotation,
    rotation_center_x,
    rotation_center_y
  );
  restore();
  return true;
}

bool render_pass_add_texture_group_effect(
  wlr_render_pass* base,
  const wlr_render_texture_options* options,
  std::size_t count,
  const EffectInstance& effect
) {
  if (!options || count == 0)
    return false;
  std::vector<EffectGroupItem> items;
  items.reserve(count);
  for (std::size_t i = 0; i < count; ++i)
    items.push_back(EffectGroupItem { .texture = &options[i] });
  return render_pass_add_group_effect(
    base,
    items.data(),
    items.size(),
    effect,
    1.0f
  );
}

bool render_pass_add_texture_effect(
  wlr_render_pass* base,
  const wlr_render_texture_options* options,
  const EffectInstance& effect
) {
  return render_pass_add_texture_group_effect(base, options, 1, effect);
}

void render_pass_add_texture(
  RenderPass* pass,
  const wlr_render_texture_options* options
) {
  if (!pass || !options || !options->texture)
    return;

  Texture* texture = wl_container_of(options->texture, texture, base);
  wlr_box dst {};
  wlr_render_texture_options_get_dst_box(options, &dst);
  if (dst.width <= 0 || dst.height <= 0)
    return;

  float alpha = wlr_render_texture_options_get_alpha(options);
  bool blend_none = (!texture->has_alpha && alpha == 1.0f)
    || options->blend_mode == WLR_RENDER_BLEND_MODE_NONE;
  setup_blend(pass, blend_none);

  Renderer* renderer = pass->renderer;
  Shader* shader = nullptr;
  if (texture->target == GL_TEXTURE_EXTERNAL_OES)
    shader = &renderer->external;
  else
    shader = texture->has_alpha ? &renderer->tex_rgba : &renderer->tex_rgbx;
  if (!shader->valid)
    return;

  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved))
    return;

  if (!wait_for_timeline(pass, options->wait_timeline, options->wait_point)) {
    egl_restore_context(&saved);
    return;
  }

  glBindTexture(texture->target, texture->tex);
  switch (options->filter_mode) {
    case WLR_SCALE_FILTER_BILINEAR:
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;
    case WLR_SCALE_FILTER_NEAREST:
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      break;
  }
  glBindTexture(texture->target, 0);
  egl_restore_context(&saved);

  auto tex_matrix = set_tex_matrix(options, texture);
  auto uv0 = Vector2Transform({ 0.0f, 0.0f }, tex_matrix);
  auto uv1 = Vector2Transform({ 1.0f, 0.0f }, tex_matrix);
  auto uv2 = Vector2Transform({ 1.0f, 1.0f }, tex_matrix);
  auto uv3 = Vector2Transform({ 0.0f, 1.0f }, tex_matrix);

  float x1 = static_cast<float>(dst.x);
  float y1 = static_cast<float>(dst.y);
  float x2 = static_cast<float>(dst.x + dst.width);
  float y2 = static_cast<float>(dst.y + dst.height);
  const float vertices[] = {
    x1, y1, uv0.x, uv0.y, x2, y1, uv1.x, uv1.y, x2, y2, uv2.x, uv2.y,
    x1, y1, uv0.x, uv0.y, x2, y2, uv2.x, uv2.y, x1, y2, uv3.x, uv3.y,
  };

  pixman_region32_t region;
  pixman_region32_init_rect(&region, dst.x, dst.y, dst.width, dst.height);
  if (options->clip)
    pixman_region32_intersect(&region, &region, options->clip);

  int rects_len = 0;
  const pixman_box32_t* rects = pixman_region32_rectangles(&region, &rects_len);

  reset_vertex_attributes();
  glUseProgram(shader->id);
  shader_set_projection(shader->mvp, pass->width, pass->height);
  if (shader->texture >= 0)
    glUniform1i(shader->texture, 0);
  glUniform4f(shader->color, 1.0f, 1.0f, 1.0f, alpha);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(texture->target, texture->tex);

  glEnableVertexAttribArray(shader->pos_attrib);
  glVertexAttribPointer(
    shader->pos_attrib,
    4,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    vertices
  );

  for (int i = 0; i < rects_len; i++) {
    flush_batch(pass);
    if (!scissor_rect(pass, &rects[i]))
      continue;
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_SCISSOR_TEST);
  }

  glDisableVertexAttribArray(shader->pos_attrib);
  glBindTexture(texture->target, 0);
  glUseProgram(0);

  pixman_region32_fini(&region);
  damage_add_box(pass, &dst);
}

static bool render_pass_submit_cb(wlr_render_pass* pass) {
  RenderPass* render_pass = wl_container_of(pass, render_pass, base);
  effect_programs_prune(render_pass->renderer);
  effect_textures_prune(render_pass->renderer);
  return render_pass_submit(render_pass);
}

static void render_pass_add_texture_cb(
  wlr_render_pass* pass,
  const wlr_render_texture_options* options
) {
  RenderPass* render_pass = wl_container_of(pass, render_pass, base);
  render_pass_add_texture(render_pass, options);
}

static void render_pass_add_rect_cb(
  wlr_render_pass* pass,
  const wlr_render_rect_options* options
) {
  RenderPass* render_pass = wl_container_of(pass, render_pass, base);
  render_pass_add_rect(render_pass, options);
}

} // namespace ura::render::raylib
