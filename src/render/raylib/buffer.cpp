#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

static void delete_buffer_gl_resources(Buffer* buffer) {
  if (!buffer || !buffer->renderer)
    return;

  if (buffer->rbo) {
    glDeleteRenderbuffers(1, &buffer->rbo);
    buffer->rbo = 0;
  }
  if (buffer->fbo) {
    glDeleteFramebuffers(1, &buffer->fbo);
    buffer->fbo = 0;
  }
  if (buffer->image != EGL_NO_IMAGE_KHR) {
    buffer->renderer->egl.destroy_image(
      buffer->renderer->egl.display,
      buffer->image
    );
    buffer->image = EGL_NO_IMAGE_KHR;
  }
}

static void buffer_finish_impl(Buffer* buffer) {
  if (!buffer)
    return;

  Renderer* renderer = buffer->renderer;
  if (renderer) {
    EglSavedContext saved {};
    if (egl_make_current(&renderer->egl, &saved)) {
      delete_buffer_gl_resources(buffer);
      egl_restore_context(&saved);
    }
  }

  wl_list_remove(&buffer->link);
  wlr_addon_finish(&buffer->addon);
  free(buffer);
}

static void handle_buffer_destroy(wlr_addon* addon) {
  Buffer* buffer = wl_container_of(addon, buffer, addon);
  buffer_finish_impl(buffer);
}

static const wlr_addon_interface buffer_addon_impl_final = {
  .name = "ura_raylib_buffer",
  .destroy = handle_buffer_destroy,
};

static int
plane_attributes(int plane, EGLint* fd, EGLint* offset, EGLint* pitch) {
  switch (plane) {
    case 0:
      *fd = EGL_DMA_BUF_PLANE0_FD_EXT;
      *offset = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
      *pitch = EGL_DMA_BUF_PLANE0_PITCH_EXT;
      return 0;
    case 1:
      *fd = EGL_DMA_BUF_PLANE1_FD_EXT;
      *offset = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
      *pitch = EGL_DMA_BUF_PLANE1_PITCH_EXT;
      return 0;
    case 2:
      *fd = EGL_DMA_BUF_PLANE2_FD_EXT;
      *offset = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
      *pitch = EGL_DMA_BUF_PLANE2_PITCH_EXT;
      return 0;
    case 3:
      *fd = EGL_DMA_BUF_PLANE3_FD_EXT;
      *offset = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
      *pitch = EGL_DMA_BUF_PLANE3_PITCH_EXT;
      return 0;
    default:
      return -1;
  }
}

static int plane_modifier_attributes(int plane, EGLint* lo, EGLint* hi) {
  switch (plane) {
    case 0:
      *lo = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
      *hi = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
      return 0;
    case 1:
      *lo = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
      *hi = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
      return 0;
    case 2:
      *lo = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
      *hi = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
      return 0;
    case 3:
      *lo = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
      *hi = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
      return 0;
    default:
      return -1;
  }
}

static EGLImageKHR create_egl_image(
  Renderer* renderer,
  wlr_dmabuf_attributes* attributes,
  bool* external_only
) {
  EglContext* egl = &renderer->egl;
  if (!egl->has_image_base || !egl->has_dmabuf_import)
    return EGL_NO_IMAGE_KHR;

  if (
    attributes->modifier != DRM_FORMAT_MOD_INVALID
    && attributes->modifier != DRM_FORMAT_MOD_LINEAR
    && !egl->has_dmabuf_modifiers
  ) {
    wlr_log(WLR_ERROR, "EGL implementation does not support DMA-BUF modifiers");
    return EGL_NO_IMAGE_KHR;
  }

  EGLint attribs[64];
  unsigned int i = 0;
  attribs[i++] = EGL_WIDTH;
  attribs[i++] = attributes->width;
  attribs[i++] = EGL_HEIGHT;
  attribs[i++] = attributes->height;
  attribs[i++] = EGL_LINUX_DRM_FOURCC_EXT;
  attribs[i++] = static_cast<EGLint>(attributes->format);

  for (int plane = 0; plane < attributes->n_planes; plane++) {
    EGLint fd_attr = 0, offset_attr = 0, pitch_attr = 0;
    if (plane_attributes(plane, &fd_attr, &offset_attr, &pitch_attr) < 0)
      break;

    attribs[i++] = fd_attr;
    attribs[i++] = attributes->fd[plane];
    attribs[i++] = offset_attr;
    attribs[i++] = static_cast<EGLint>(attributes->offset[plane]);
    attribs[i++] = pitch_attr;
    attribs[i++] = static_cast<EGLint>(attributes->stride[plane]);

    if (
      egl->has_dmabuf_modifiers
      && attributes->modifier != DRM_FORMAT_MOD_INVALID
    ) {
      EGLint lo_attr = 0, hi_attr = 0;
      if (plane_modifier_attributes(plane, &lo_attr, &hi_attr) < 0)
        break;
      attribs[i++] = lo_attr;
      attribs[i++] = static_cast<EGLint>(attributes->modifier & 0xFFFFFFFF);
      attribs[i++] = hi_attr;
      attribs[i++] = static_cast<EGLint>(attributes->modifier >> 32);
    }
  }

  attribs[i++] = EGL_IMAGE_PRESERVED_KHR;
  attribs[i++] = EGL_TRUE;
  attribs[i++] = EGL_NONE;

  EGLImageKHR image = egl->create_image(
    egl->display,
    EGL_NO_CONTEXT,
    EGL_LINUX_DMA_BUF_EXT,
    nullptr,
    attribs
  );
  if (image == EGL_NO_IMAGE_KHR) {
    wlr_log(WLR_ERROR, "eglCreateImageKHR failed");
    return EGL_NO_IMAGE_KHR;
  }

  *external_only = !wlr_drm_format_set_has(
    &egl->render_formats,
    attributes->format,
    attributes->modifier
  );
  return image;
}

Buffer* buffer_get(Renderer* renderer, wlr_buffer* wlr_buffer) {
  if (!renderer || !wlr_buffer)
    return nullptr;

  wlr_addon* addon =
    wlr_addon_find(&wlr_buffer->addons, renderer, &buffer_addon_impl_final);
  if (addon) {
    Buffer* buffer = wl_container_of(addon, buffer, addon);
    return buffer;
  }

  wlr_dmabuf_attributes attributes {};
  if (!wlr_buffer_get_dmabuf(wlr_buffer, &attributes))
    return nullptr;

  Buffer* buffer = static_cast<Buffer*>(calloc(1, sizeof(Buffer)));
  if (!buffer)
    return nullptr;

  buffer->renderer = renderer;
  buffer->buffer = wlr_buffer;
  buffer->width = wlr_buffer->width;
  buffer->height = wlr_buffer->height;
  buffer->format = attributes.format;
  buffer->image = EGL_NO_IMAGE_KHR;

  buffer->image =
    create_egl_image(renderer, &attributes, &buffer->external_only);
  if (buffer->image == EGL_NO_IMAGE_KHR) {
    free(buffer);
    return nullptr;
  }

  wlr_addon_init(
    &buffer->addon,
    &wlr_buffer->addons,
    renderer,
    &buffer_addon_impl_final
  );
  wl_list_insert(&renderer->buffers, &buffer->link);

  wlr_log(
    WLR_DEBUG,
    "Imported DMA-BUF buffer %dx%d (format 0x%x, modifier 0x%llx)%s",
    buffer->width,
    buffer->height,
    buffer->format,
    static_cast<unsigned long long>(attributes.modifier),
    buffer->external_only ? ", external-only" : ""
  );

  return buffer;
}

bool buffer_ensure_render_target(Buffer* buffer) {
  if (!buffer)
    return false;
  if (buffer->fbo)
    return true;
  if (buffer->external_only) {
    wlr_log(WLR_ERROR, "Cannot render to external-only DMA-BUF");
    return false;
  }

  Renderer* renderer = buffer->renderer;
  EglSavedContext saved {};
  if (!egl_make_current(&renderer->egl, &saved))
    return false;

  while (glGetError() != GL_NO_ERROR) {}
  glGenRenderbuffers(1, &buffer->rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, buffer->rbo);
  renderer->egl.image_target_renderbuffer(GL_RENDERBUFFER, buffer->image);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  if (glGetError() != GL_NO_ERROR) {
    wlr_log(WLR_ERROR, "Failed to attach EGLImage to renderbuffer");
    delete_buffer_gl_resources(buffer);
    egl_restore_context(&saved);
    return false;
  }

  glGenFramebuffers(1, &buffer->fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo);
  glFramebufferRenderbuffer(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_RENDERBUFFER,
    buffer->rbo
  );
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  egl_restore_context(&saved);

  if (status != GL_FRAMEBUFFER_COMPLETE) {
    wlr_log(WLR_ERROR, "DMA-BUF framebuffer is incomplete: 0x%x", status);
    delete_buffer_gl_resources(buffer);
    return false;
  }

  return true;
}

void buffer_finish(Buffer* buffer) {
  buffer_finish_impl(buffer);
}

} // namespace ura::render::raylib
