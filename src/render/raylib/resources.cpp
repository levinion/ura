#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>
#include <initializer_list>

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

// The DRM formats are little-endian while GL formats are byte-oriented. Most
// 32-bit formats are normalized to GL_RGBA in create_texture_object(); the
// upload_swizzle field describes how to translate DRM memory order to that
// layout. For example DRM_FORMAT_ARGB8888 is laid out as B,G,R,A in memory.
static const GlFormat formats[] = {
  { DRM_FORMAT_ARGB8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    true,
    { 2, 1, 0, 3 } },
  { DRM_FORMAT_XRGB8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    false,
    { 2, 1, 0, 3 } },
  { DRM_FORMAT_ABGR8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    true,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_XBGR8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    false,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_RGBA8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    true,
    { 3, 2, 1, 0 } },
  { DRM_FORMAT_RGBX8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    false,
    { 3, 2, 1, 0 } },
  { DRM_FORMAT_BGRA8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    true,
    { 1, 2, 3, 0 } },
  { DRM_FORMAT_BGRX8888,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    false,
    { 1, 2, 3, 0 } },
  { DRM_FORMAT_BGR888,
    GL_RGB,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    false,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_RGB888,
    GL_RGB,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    false,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_RGB565,
    GL_RGB,
    GL_RGB,
    GL_UNSIGNED_SHORT_5_6_5,
    false,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_ABGR2101010,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_INT_2_10_10_10_REV_EXT,
    true,
    { 0, 1, 2, 3 } },
  { DRM_FORMAT_XBGR2101010,
    GL_RGBA,
    GL_RGBA,
    GL_UNSIGNED_INT_2_10_10_10_REV_EXT,
    false,
    { 0, 1, 2, 3 } },
};

const GlFormat* gl_format_from_drm(uint32_t format) {
  for (const auto& fmt : formats) {
    if (fmt.drm_format == format)
      return &fmt;
  }
  return nullptr;
}

const GlFormat* gl_format_from_gl(GLenum format, GLenum type, bool alpha) {
  for (const auto& fmt : formats) {
    if (fmt.format != format || fmt.type != type)
      continue;
    if (fmt.has_alpha != alpha)
      continue;
    return &fmt;
  }
  return nullptr;
}

bool gl_format_is_ycbcr(uint32_t format) {
  switch (format) {
    case DRM_FORMAT_NV12:
    case DRM_FORMAT_NV21:
    case DRM_FORMAT_NV16:
    case DRM_FORMAT_NV61:
    case DRM_FORMAT_NV24:
    case DRM_FORMAT_NV42:
    case DRM_FORMAT_YUV410:
    case DRM_FORMAT_YVU410:
    case DRM_FORMAT_YUV411:
    case DRM_FORMAT_YVU411:
    case DRM_FORMAT_YUV420:
    case DRM_FORMAT_YVU420:
    case DRM_FORMAT_YUV422:
    case DRM_FORMAT_YVU422:
    case DRM_FORMAT_YUV444:
    case DRM_FORMAT_YVU444:
    case DRM_FORMAT_P010:
    case DRM_FORMAT_P012:
    case DRM_FORMAT_P016:
      return true;
    default:
      return false;
  }
}

bool gl_format_has_alpha(uint32_t format) {
  if (const auto* fmt = gl_format_from_drm(format))
    return fmt->has_alpha;
  if (gl_format_is_ycbcr(format))
    return false;
  // Unknown formats are treated as transparent to force blending.
  return true;
}

static void
add_format(wlr_drm_format_set* set, uint32_t format, uint64_t modifier) {
  wlr_drm_format_set_add(set, format, modifier);
}

static void add_simple_format(wlr_drm_format_set* set, uint32_t format) {
  add_format(set, format, DRM_FORMAT_MOD_INVALID);
  add_format(set, format, DRM_FORMAT_MOD_LINEAR);
}

static void init_shm_formats(EglContext* egl) {
  static const uint32_t shm_formats[] = {
    DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888, DRM_FORMAT_ABGR8888,
    DRM_FORMAT_XBGR8888, DRM_FORMAT_RGBA8888, DRM_FORMAT_RGBX8888,
    DRM_FORMAT_BGRA8888, DRM_FORMAT_BGRX8888, DRM_FORMAT_RGB565,
  };

  for (uint32_t format : shm_formats) {
    if (!gl_format_from_drm(format))
      continue;
    add_simple_format(&egl->shm_formats, format);
  }
}

static void init_dmabuf_fallback_formats(EglContext* egl) {
  wlr_log(
    WLR_INFO,
    "EGL DMA-BUF modifier query unavailable, using ARGB8888/XRGB8888"
  );
  for (uint32_t format : { DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888 }) {
    add_simple_format(&egl->texture_formats, format);
    add_simple_format(&egl->render_formats, format);
  }
}

static void init_dmabuf_formats(EglContext* egl) {
  if (!egl->has_dmabuf_import) {
    wlr_log(WLR_INFO, "EGL DMA-BUF import is not available");
    return;
  }

  if (
    !egl->has_dmabuf_modifiers || !egl->query_dmabuf_formats
    || !egl->query_dmabuf_modifiers
  ) {
    init_dmabuf_fallback_formats(egl);
    return;
  }

  EGLint formats_len = 0;
  if (
    !egl->query_dmabuf_formats(egl->display, 0, nullptr, &formats_len)
    || formats_len <= 0
  ) {
    wlr_log(WLR_ERROR, "Failed to query EGL DMA-BUF formats");
    init_dmabuf_fallback_formats(egl);
    return;
  }

  auto egl_formats = static_cast<EGLint*>(calloc(formats_len, sizeof(EGLint)));
  if (!egl_formats) {
    init_dmabuf_fallback_formats(egl);
    return;
  }

  if (!egl->query_dmabuf_formats(
        egl->display,
        formats_len,
        egl_formats,
        &formats_len
      )) {
    wlr_log(WLR_ERROR, "Failed to query EGL DMA-BUF formats");
    free(egl_formats);
    init_dmabuf_fallback_formats(egl);
    return;
  }

  for (EGLint i = 0; i < formats_len; i++) {
    uint32_t format = static_cast<uint32_t>(egl_formats[i]);

    EGLint modifiers_len = 0;
    if (!egl->query_dmabuf_modifiers(
          egl->display,
          format,
          0,
          nullptr,
          nullptr,
          &modifiers_len
        )) {
      modifiers_len = 0;
    }

    uint64_t* modifiers = nullptr;
    EGLBoolean* external_only = nullptr;
    if (modifiers_len > 0) {
      modifiers =
        static_cast<uint64_t*>(calloc(modifiers_len, sizeof(uint64_t)));
      external_only =
        static_cast<EGLBoolean*>(calloc(modifiers_len, sizeof(EGLBoolean)));
      if (!modifiers || !external_only) {
        free(modifiers);
        free(external_only);
        modifiers_len = 0;
      } else if (!egl->query_dmabuf_modifiers(
                   egl->display,
                   format,
                   modifiers_len,
                   modifiers,
                   external_only,
                   &modifiers_len
                 )) {
        free(modifiers);
        free(external_only);
        modifiers_len = 0;
      }
    }

    bool all_external_only = modifiers_len > 0;
    for (EGLint j = 0; j < modifiers_len; j++) {
      add_format(&egl->texture_formats, format, modifiers[j]);
      if (!external_only[j]) {
        add_format(&egl->render_formats, format, modifiers[j]);
        all_external_only = false;
      }
    }

    // EGL always supports implicit modifiers for imported images.
    add_format(&egl->texture_formats, format, DRM_FORMAT_MOD_INVALID);
    if (modifiers_len == 0 || !all_external_only)
      add_format(&egl->render_formats, format, DRM_FORMAT_MOD_INVALID);

    if (modifiers_len == 0) {
      add_format(&egl->texture_formats, format, DRM_FORMAT_MOD_LINEAR);
      add_format(&egl->render_formats, format, DRM_FORMAT_MOD_LINEAR);
    }

    free(modifiers);
    free(external_only);
  }

  free(egl_formats);
  wlr_log(
    WLR_DEBUG,
    "Renderer advertises %zu DMA-BUF texture formats and %zu render formats",
    egl->texture_formats.len,
    egl->render_formats.len
  );
}

bool renderer_init_surface_formats(Renderer* renderer) {
  init_shm_formats(&renderer->egl);
  init_dmabuf_formats(&renderer->egl);
  return renderer->egl.shm_formats.len > 0;
}

void renderer_finish_surface_formats(Renderer* renderer) {
  wlr_drm_format_set_finish(&renderer->egl.texture_formats);
  wlr_drm_format_set_finish(&renderer->egl.render_formats);
  wlr_drm_format_set_finish(&renderer->egl.shm_formats);
}

} // namespace ura::render::raylib
