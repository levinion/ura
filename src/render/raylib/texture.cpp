#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>
#include <cstring>

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

static bool texture_update_from_buffer(
  wlr_texture* texture,
  wlr_buffer* buffer,
  const pixman_region32_t* damage
);
static bool texture_read_pixels(
  wlr_texture* texture,
  const wlr_texture_read_pixels_options* options
);
static uint32_t texture_preferred_read_format(wlr_texture* texture);
static void handle_texture_destroy(wlr_texture* texture);

static const wlr_texture_impl texture_impl = {
  .update_from_buffer = texture_update_from_buffer,
  .read_pixels = texture_read_pixels,
  .preferred_read_format = texture_preferred_read_format,
  .destroy = handle_texture_destroy,
};

static size_t gl_format_bytes_per_pixel(const GlFormat* format) {
  if (!format)
    return 0;
  switch (format->type) {
    case GL_UNSIGNED_BYTE:
      if (format->format == GL_RGBA || format->format == GL_BGRA_EXT)
        return 4;
      if (format->format == GL_RGB)
        return 3;
      return 1;
    case GL_UNSIGNED_SHORT_5_6_5:
      return 2;
    case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
      return 4;
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

static void upload_texture_rows(
  GLenum target,
  GLenum format,
  GLenum type,
  int width,
  int height,
  const void* data,
  size_t stride,
  size_t bytes_per_pixel,
  int upload_y,
  int upload_height
) {
  if (upload_height <= 0 || width <= 0)
    return;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (stride == static_cast<size_t>(width) * bytes_per_pixel) {
    glTexSubImage2D(
      target,
      0,
      0,
      upload_y,
      width,
      upload_height,
      format,
      type,
      data
    );
    return;
  }

  auto bytes = static_cast<const uint8_t*>(data);
  for (int y = 0; y < upload_height; y++) {
    glTexSubImage2D(
      target,
      0,
      0,
      upload_y + y,
      width,
      1,
      format,
      type,
      bytes + static_cast<size_t>(y) * stride
    );
  }
}

static GLuint create_texture_object(
  Renderer* renderer,
  const GlFormat* format,
  int width,
  int height,
  const void* data,
  size_t stride
) {
  while (glGetError() != GL_NO_ERROR) {}
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLint internal_format =
    format->internal_format ? format->internal_format : format->format;
  GLenum upload_format = format->format;
  GLenum upload_type = format->type;
  const void* upload_data = data;
  size_t upload_stride = stride;

  uint8_t* converted = nullptr;
  if (data && format_needs_swizzle(format)) {
    converted = static_cast<uint8_t*>(
      malloc(static_cast<size_t>(width) * static_cast<size_t>(height) * 4)
    );
    if (!converted) {
      glDeleteTextures(1, &texture);
      return 0;
    }
    auto src = static_cast<const uint8_t*>(data);
    for (int y = 0; y < height; y++) {
      swizzle_row(
        src + static_cast<size_t>(y) * stride,
        converted + static_cast<size_t>(y) * width * 4,
        width,
        format->upload_swizzle
      );
    }
    upload_format = GL_RGBA;
    upload_type = GL_UNSIGNED_BYTE;
    upload_data = converted;
    upload_stride = static_cast<size_t>(width) * 4;
  }

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    internal_format,
    width,
    height,
    0,
    upload_format,
    upload_type,
    nullptr
  );

  if (upload_data) {
    size_t bpp = (upload_format == GL_RGBA && upload_type == GL_UNSIGNED_BYTE)
      ? 4
      : gl_format_bytes_per_pixel(format);
    upload_texture_rows(
      GL_TEXTURE_2D,
      upload_format,
      upload_type,
      width,
      height,
      upload_data,
      upload_stride,
      bpp,
      0,
      height
    );
  }
  free(converted);

  glBindTexture(GL_TEXTURE_2D, 0);
  if (glGetError() != GL_NO_ERROR) {
    wlr_log(WLR_ERROR, "Failed to create GL texture for buffer");
    glDeleteTextures(1, &texture);
    return 0;
  }
  return texture;
}

static Texture* texture_create(
  Renderer* renderer,
  uint32_t width,
  uint32_t height,
  GLenum target,
  bool has_alpha,
  bool mutable_texture
) {
  auto* texture = static_cast<Texture*>(calloc(1, sizeof(Texture)));
  if (!texture)
    return nullptr;

  texture->renderer = renderer;
  texture->target = target;
  texture->has_alpha = has_alpha;
  texture->mutable_texture = mutable_texture;
  texture->drm_format = DRM_FORMAT_INVALID;
  wlr_texture_init(
    &texture->base,
    &renderer->base,
    &texture_impl,
    width,
    height
  );
  wl_list_insert(&renderer->textures, &texture->link);
  return texture;
}

static Texture* texture_from_dmabuf(Renderer* renderer, wlr_buffer* buffer) {
  Buffer* imported = buffer_get(renderer, buffer);
  if (!imported)
    return nullptr;

  wlr_dmabuf_attributes attributes {};
  if (!wlr_buffer_get_dmabuf(buffer, &attributes))
    return nullptr;

  GLenum target =
    imported->external_only ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;

  if (
    target == GL_TEXTURE_EXTERNAL_OES && !renderer->egl.has_egl_image_external
  )
    return nullptr;

  Texture* texture = texture_create(
    renderer,
    attributes.width,
    attributes.height,
    target,
    gl_format_has_alpha(attributes.format),
    false
  );
  if (!texture)
    return nullptr;

  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved)) {
    texture_destroy(texture);
    return nullptr;
  }

  while (glGetError() != GL_NO_ERROR) {}
  glGenTextures(1, &texture->tex);
  glBindTexture(target, texture->tex);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  renderer->egl.image_target_texture(target, imported->image);
  glBindTexture(target, 0);

  if (glGetError() != GL_NO_ERROR) {
    wlr_log(WLR_ERROR, "Failed to import DMA-BUF as GL texture");
    egl_restore_context(&saved);
    texture_destroy(texture);
    return nullptr;
  }

  egl_restore_context(&saved);

  texture->buffer = imported;
  wlr_buffer_lock(buffer);
  return texture;
}

static Texture* texture_from_pixels(
  Renderer* renderer,
  wlr_buffer* buffer,
  const void* data,
  uint32_t format,
  size_t stride
) {
  const GlFormat* gl_format = gl_format_from_drm(format);
  if (!gl_format) {
    wlr_log(WLR_ERROR, "Unsupported SHM pixel format 0x%x", format);
    return nullptr;
  }

  Texture* texture = texture_create(
    renderer,
    buffer->width,
    buffer->height,
    GL_TEXTURE_2D,
    gl_format->has_alpha,
    true
  );
  if (!texture)
    return nullptr;
  texture->drm_format = format;

  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved)) {
    texture_destroy(texture);
    return nullptr;
  }

  texture->tex = create_texture_object(
    renderer,
    gl_format,
    buffer->width,
    buffer->height,
    data,
    stride
  );

  egl_restore_context(&saved);

  if (!texture->tex) {
    texture_destroy(texture);
    return nullptr;
  }
  return texture;
}

Texture* texture_from_buffer(Renderer* renderer, wlr_buffer* buffer) {
  if (!renderer || !buffer)
    return nullptr;

  wlr_dmabuf_attributes attributes {};
  if (wlr_buffer_get_dmabuf(buffer, &attributes))
    return texture_from_dmabuf(renderer, buffer);

  void* data = nullptr;
  uint32_t format = DRM_FORMAT_INVALID;
  size_t stride = 0;
  if (!wlr_buffer_begin_data_ptr_access(
        buffer,
        WLR_BUFFER_DATA_PTR_ACCESS_READ,
        &data,
        &format,
        &stride
      )) {
    wlr_log(WLR_ERROR, "Failed to access buffer pixels");
    return nullptr;
  }

  Texture* texture =
    texture_from_pixels(renderer, buffer, data, format, stride);
  wlr_buffer_end_data_ptr_access(buffer);
  return texture;
}

static bool texture_update_from_buffer(
  wlr_texture* wlr_texture,
  wlr_buffer* buffer,
  const pixman_region32_t* damage
) {
  Texture* texture = wl_container_of(wlr_texture, texture, base);
  if (!texture->mutable_texture || texture->target != GL_TEXTURE_2D)
    return false;

  void* data = nullptr;
  uint32_t format = DRM_FORMAT_INVALID;
  size_t stride = 0;
  if (!wlr_buffer_begin_data_ptr_access(
        buffer,
        WLR_BUFFER_DATA_PTR_ACCESS_READ,
        &data,
        &format,
        &stride
      ))
    return false;

  if (format != texture->drm_format) {
    wlr_buffer_end_data_ptr_access(buffer);
    return false;
  }

  const GlFormat* gl_format = gl_format_from_drm(format);
  if (!gl_format) {
    wlr_buffer_end_data_ptr_access(buffer);
    return false;
  }
  size_t bpp = gl_format_bytes_per_pixel(gl_format);
  if (bpp == 0) {
    wlr_buffer_end_data_ptr_access(buffer);
    return false;
  }

  Renderer* renderer = texture->renderer;
  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved)) {
    wlr_buffer_end_data_ptr_access(buffer);
    return false;
  }

  glBindTexture(GL_TEXTURE_2D, texture->tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  bool swizzle = format_needs_swizzle(gl_format);
  uint8_t* row = nullptr;
  if (swizzle) {
    row = static_cast<uint8_t*>(
      malloc(static_cast<size_t>(texture->base.width) * 4)
    );
    if (!row) {
      glBindTexture(GL_TEXTURE_2D, 0);
      egl_restore_context(&saved);
      wlr_buffer_end_data_ptr_access(buffer);
      return false;
    }
  }

  int rects_len = 0;
  const pixman_box32_t* rects = pixman_region32_rectangles(damage, &rects_len);
  auto bytes = static_cast<const uint8_t*>(data);
  for (int i = 0; i < rects_len; i++) {
    int x1 = rects[i].x1;
    int y1 = rects[i].y1;
    int x2 = rects[i].x2;
    int y2 = rects[i].y2;
    if (x1 < 0)
      x1 = 0;
    if (y1 < 0)
      y1 = 0;
    if (x2 > static_cast<int>(texture->base.width))
      x2 = static_cast<int>(texture->base.width);
    if (y2 > static_cast<int>(texture->base.height))
      y2 = static_cast<int>(texture->base.height);
    if (x1 >= x2 || y1 >= y2)
      continue;

    for (int y = y1; y < y2; y++) {
      const void* src =
        bytes + static_cast<size_t>(y) * stride + static_cast<size_t>(x1) * bpp;
      if (swizzle) {
        swizzle_row(
          static_cast<const uint8_t*>(src),
          row,
          x2 - x1,
          gl_format->upload_swizzle
        );
        src = row;
      }
      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        x1,
        y,
        x2 - x1,
        1,
        swizzle ? GL_RGBA : gl_format->format,
        swizzle ? GL_UNSIGNED_BYTE : gl_format->type,
        src
      );
    }
  }

  free(row);
  glBindTexture(GL_TEXTURE_2D, 0);
  egl_restore_context(&saved);
  wlr_buffer_end_data_ptr_access(buffer);
  return true;
}

static bool bind_texture_fbo(Texture* texture, GLuint* fbo) {
  *fbo = 0;
  if (texture->target != GL_TEXTURE_2D)
    return false;

  glGenFramebuffers(1, fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    texture->tex,
    0
  );
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    wlr_log(WLR_ERROR, "Texture framebuffer is incomplete: 0x%x", status);
    glDeleteFramebuffers(1, fbo);
    *fbo = 0;
    return false;
  }
  return true;
}

static bool texture_read_pixels(
  wlr_texture* wlr_texture,
  const wlr_texture_read_pixels_options* options
) {
  Texture* texture = wl_container_of(wlr_texture, texture, base);
  if (texture->target != GL_TEXTURE_2D)
    return false;

  const GlFormat* gl_format = gl_format_from_drm(options->format);
  if (!gl_format) {
    wlr_log(WLR_ERROR, "Unsupported read format 0x%x", options->format);
    return false;
  }

  wlr_box src {};
  wlr_texture_read_pixels_options_get_src_box(options, wlr_texture, &src);
  void* data = wlr_texture_read_pixel_options_get_data(options);
  if (!data)
    return false;

  Renderer* renderer = texture->renderer;
  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved))
    return false;

  GLuint fbo = 0;
  if (!bind_texture_fbo(texture, &fbo)) {
    egl_restore_context(&saved);
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFinish();
  glGetError();
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  if (gl_format->format != GL_RGBA || gl_format->type != GL_UNSIGNED_BYTE) {
    wlr_log(WLR_ERROR, "Unsupported readback format 0x%x", options->format);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    egl_restore_context(&saved);
    return false;
  }

  auto row = static_cast<uint8_t*>(malloc(static_cast<size_t>(src.width) * 4));
  if (!row) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    egl_restore_context(&saved);
    return false;
  }

  auto out = static_cast<uint8_t*>(data);
  for (int y = 0; y < src.height; y++) {
    glReadPixels(
      src.x,
      src.y + y,
      src.width,
      1,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      row
    );
    uint8_t* dst = out + static_cast<size_t>(y) * options->stride;
    for (int x = 0; x < src.width; x++) {
      const uint8_t* in = row + static_cast<size_t>(x) * 4;
      uint8_t* pixel = dst + static_cast<size_t>(x) * 4;
      pixel[gl_format->upload_swizzle[0]] = in[0];
      pixel[gl_format->upload_swizzle[1]] = in[1];
      pixel[gl_format->upload_swizzle[2]] = in[2];
      pixel[gl_format->upload_swizzle[3]] = in[3];
    }
  }
  free(row);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  egl_restore_context(&saved);
  return glGetError() == GL_NO_ERROR;
}

static uint32_t texture_preferred_read_format(wlr_texture* wlr_texture) {
  Texture* texture = wl_container_of(wlr_texture, texture, base);
  if (texture->target != GL_TEXTURE_2D)
    return DRM_FORMAT_INVALID;

  Renderer* renderer = texture->renderer;
  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved))
    return DRM_FORMAT_INVALID;

  GLuint fbo = 0;
  if (!bind_texture_fbo(texture, &fbo)) {
    egl_restore_context(&saved);
    return DRM_FORMAT_INVALID;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLint gl_format = -1, gl_type = -1, alpha_bits = -1;
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &gl_format);
  glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &gl_type);
  glGetIntegerv(GL_ALPHA_BITS, &alpha_bits);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &fbo);
  egl_restore_context(&saved);

  const GlFormat* fmt = gl_format_from_gl(
    static_cast<GLenum>(gl_format),
    static_cast<GLenum>(gl_type),
    alpha_bits > 0
  );
  if (fmt)
    return fmt->drm_format;
  if (renderer->egl.has_bgra)
    return DRM_FORMAT_XRGB8888;
  return DRM_FORMAT_INVALID;
}

static void texture_destroy_impl(Texture* texture) {
  if (!texture)
    return;

  Renderer* renderer = texture->renderer;
  EglSavedContext saved {};
  bool current = egl_make_current(&renderer->egl, &saved);
  if (current && texture->tex)
    glDeleteTextures(1, &texture->tex);
  if (current)
    egl_restore_context(&saved);

  wl_list_remove(&texture->link);
  if (texture->buffer)
    wlr_buffer_unlock(texture->buffer->buffer);
  free(texture);
}

static void handle_texture_destroy(wlr_texture* wlr_texture) {
  Texture* texture = wl_container_of(wlr_texture, texture, base);
  texture_destroy_impl(texture);
}

void texture_destroy(Texture* texture) {
  texture_destroy_impl(texture);
}

} // namespace ura::render::raylib
