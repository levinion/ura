#include "ura/render/raylib/renderer.hpp"

#include <unistd.h>

#include <cstdlib>
#include <cstring>

extern "C" {
#include <wlr/util/box.h>
#include <wlr/util/log.h>
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

static void mat3_identity(float m[9]) {
  static const float identity[9] = {
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  memcpy(m, identity, sizeof(identity));
}

static void mat3_multiply(float out[9], const float a[9], const float b[9]) {
  float product[9];
  product[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
  product[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
  product[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
  product[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
  product[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
  product[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
  product[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
  product[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
  product[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
  memcpy(out, product, sizeof(product));
}

static void mat3_translate(float m[9], float x, float y) {
  float translate[9] = {
    1.0f, 0.0f, x, 0.0f, 1.0f, y, 0.0f, 0.0f, 1.0f,
  };
  mat3_multiply(m, m, translate);
}

static void mat3_scale(float m[9], float x, float y) {
  float scale[9] = {
    x, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  mat3_multiply(m, m, scale);
}

static const float* transform_matrix(enum wl_output_transform transform) {
  static const float normal[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f };
  static const float rotate90[9] = { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f };
  static const float rotate180[9] = { -1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
                                      0.0f,  0.0f, 0.0f, 1.0f };
  static const float rotate270[9] = { 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f,  0.0f, 1.0f };
  static const float flipped[9] = { -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                    0.0f,  0.0f, 0.0f, 1.0f };
  static const float flipped90[9] = { 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f };
  static const float flipped180[9] = { 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f };
  static const float flipped270[9] = { 0.0f, -1.0f, 0.0f, -1.0f, 0.0f,
                                       0.0f, 0.0f,  0.0f, 1.0f };

  switch (transform) {
    case WL_OUTPUT_TRANSFORM_NORMAL:
      return normal;
    case WL_OUTPUT_TRANSFORM_90:
      return rotate90;
    case WL_OUTPUT_TRANSFORM_180:
      return rotate180;
    case WL_OUTPUT_TRANSFORM_270:
      return rotate270;
    case WL_OUTPUT_TRANSFORM_FLIPPED:
      return flipped;
    case WL_OUTPUT_TRANSFORM_FLIPPED_90:
      return flipped90;
    case WL_OUTPUT_TRANSFORM_FLIPPED_180:
      return flipped180;
    case WL_OUTPUT_TRANSFORM_FLIPPED_270:
      return flipped270;
  }
  return normal;
}

static void mat3_transform(float m[9], enum wl_output_transform transform) {
  mat3_multiply(m, m, transform_matrix(transform));
}

static void set_tex_matrix(
  const wlr_render_texture_options* options,
  const Texture* texture,
  float matrix[9]
) {
  wlr_fbox src {};
  wlr_render_texture_options_get_src_box(options, &src);
  src.x /= texture->base.width;
  src.y /= texture->base.height;
  src.width /= texture->base.width;
  src.height /= texture->base.height;

  mat3_identity(matrix);
  mat3_translate(matrix, src.x, src.y);
  mat3_scale(matrix, src.width, src.height);
  mat3_translate(matrix, 0.5f, 0.5f);
  if (options->transform & WL_OUTPUT_TRANSFORM_90)
    mat3_transform(matrix, wlr_output_transform_invert(options->transform));
  else
    mat3_transform(matrix, options->transform);
  mat3_translate(matrix, -0.5f, -0.5f);
}

static void
texcoord(const float matrix[9], float u, float v, float* out_u, float* out_v) {
  *out_u = matrix[0] * u + matrix[3] * v + matrix[6];
  *out_v = matrix[1] * u + matrix[4] * v + matrix[7];
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

static int format_bpp(uint32_t format) {
  switch (format) {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_ABGR2101010:
    case DRM_FORMAT_XBGR2101010:
      return 4;
    case DRM_FORMAT_RGB565:
      return 2;
    default:
      return 0;
  }
}

static bool format_needs_swizzle(const GlFormat* format) {
  return format->type == GL_UNSIGNED_BYTE
    && (format->upload_swizzle[0] != 0 || format->upload_swizzle[1] != 1
        || format->upload_swizzle[2] != 2 || format->upload_swizzle[3] != 3);
}

static void swizzle_row(
  const uint8_t* src,
  uint8_t* dst,
  int width,
  const uint8_t swizzle[4]
) {
  for (int x = 0; x < width; x++) {
    const uint8_t* in = src + static_cast<size_t>(x) * 4;
    uint8_t* out = dst + static_cast<size_t>(x) * 4;
    out[0] = in[swizzle[0]];
    out[1] = in[swizzle[1]];
    out[2] = in[swizzle[2]];
    out[3] = in[swizzle[3]];
  }
}

static bool create_shm_target(RenderPass* pass, const GlFormat* format) {
  while (glGetError() != GL_NO_ERROR) {}
  int bpp = format_bpp(pass->shm_format);
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
  bool swizzle = format_needs_swizzle(format);
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
      swizzle_row(
        static_cast<const uint8_t*>(src),
        row,
        pass->width,
        format->upload_swizzle
      );
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
    wlr_log(WLR_ERROR, "SHM framebuffer is incomplete: 0x%x", status);
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

  int bpp = format_bpp(pass->shm_format);
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
    wlr_log(WLR_ERROR, "Failed to create EGL native fence sync");
    return false;
  }

  glFlush();
  int fd = egl->dup_native_fence_fd(egl->display, sync);
  egl->destroy_sync(egl->display, sync);
  if (fd < 0) {
    wlr_log(WLR_ERROR, "Failed to export EGL native fence");
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
      wlr_log(WLR_INFO, "SHM render target is read-only");
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
      wlr_log(WLR_ERROR, "Failed to read back SHM render target");
      ok = false;
    }
  }

  if (pass->signal_timeline) {
    if (!signal_timeline(pass)) {
      wlr_log(WLR_ERROR, "Failed to signal render timeline");
      ok = false;
    }
  } else {
    glFlush();
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
    wlr_log(WLR_ERROR, "Failed to export render wait timeline");
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
    wlr_log(WLR_ERROR, "Failed to create EGL wait fence");
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

  float tex_matrix[9];
  set_tex_matrix(options, texture, tex_matrix);

  float u0, v0, u1, v1, u2, v2, u3, v3;
  texcoord(tex_matrix, 0.0f, 0.0f, &u0, &v0);
  texcoord(tex_matrix, 1.0f, 0.0f, &u1, &v1);
  texcoord(tex_matrix, 1.0f, 1.0f, &u2, &v2);
  texcoord(tex_matrix, 0.0f, 1.0f, &u3, &v3);

  float x1 = static_cast<float>(dst.x);
  float y1 = static_cast<float>(dst.y);
  float x2 = static_cast<float>(dst.x + dst.width);
  float y2 = static_cast<float>(dst.y + dst.height);
  const float vertices[] = {
    x1, y1, u0, v0, x2, y1, u1, v1, x2, y2, u2, v2,
    x1, y1, u0, v0, x2, y2, u2, v2, x1, y2, u3, v3,
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
