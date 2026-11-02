#include "ura/render/raylib/renderer.hpp"

#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>
#include <xf86drm.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

bool egl_has_extension(const char* extensions, const char* name) {
  if (!extensions || !name)
    return false;
  size_t name_len = strlen(name);
  const char* end = extensions + strlen(extensions);
  while (extensions < end) {
    if (*extensions == ' ') {
      extensions++;
      continue;
    }
    size_t len = strcspn(extensions, " ");
    if (len == name_len && strncmp(extensions, name, len) == 0)
      return true;
    extensions += len;
  }
  return false;
}

static int open_render_node() {
  int devices_len = drmGetDevices2(0, nullptr, 0);
  if (devices_len < 0) {
    wlr_log(WLR_ERROR, "drmGetDevices2 failed: %s", strerror(-devices_len));
    return -1;
  }

  auto devices =
    static_cast<drmDevice**>(calloc(devices_len, sizeof(drmDevice*)));
  if (!devices)
    return -1;

  devices_len = drmGetDevices2(0, devices, devices_len);
  int fd = -1;
  if (devices_len < 0) {
    wlr_log(WLR_ERROR, "drmGetDevices2 failed: %s", strerror(-devices_len));
    free(devices);
    return -1;
  }

  for (int i = 0; i < devices_len; i++) {
    drmDevice* device = devices[i];
    if ((device->available_nodes & (1 << DRM_NODE_RENDER)) == 0)
      continue;
    const char* path = device->nodes[DRM_NODE_RENDER];
    if (!path)
      continue;
    fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd >= 0) {
      wlr_log(WLR_INFO, "Using DRM render node %s for raylib renderer", path);
      break;
    }
    wlr_log(WLR_ERROR, "Failed to open %s: %s", path, strerror(errno));
  }

  for (int i = 0; i < devices_len; i++) drmFreeDevice(&devices[i]);
  free(devices);
  return fd;
}

static void resolve_drm_fd(EglContext* egl, wlr_backend* backend) {
  egl->drm_fd = -1;
  egl->owns_drm_fd = false;

  int backend_fd = wlr_backend_get_drm_fd(backend);
  if (backend_fd >= 0) {
    egl->drm_fd = backend_fd;
    return;
  }

  const char* device = getenv("WLR_RENDER_DRM_DEVICE");
  if (device && *device) {
    int fd = open(device, O_RDWR | O_CLOEXEC);
    if (fd >= 0) {
      egl->drm_fd = fd;
      egl->owns_drm_fd = true;
      return;
    }
    wlr_log(WLR_ERROR, "Failed to open %s: %s", device, strerror(errno));
  }

  if ((backend->buffer_caps & WLR_BUFFER_CAP_DMABUF) == 0)
    return;

  int fd = open_render_node();
  if (fd >= 0) {
    egl->drm_fd = fd;
    egl->owns_drm_fd = true;
  }
}

static bool choose_config(EglContext* egl) {
  const EGLint attrs[] = {
    EGL_SURFACE_TYPE,
    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES2_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_NONE,
  };

  EGLint count = 0;
  if (
    eglChooseConfig(egl->display, attrs, &egl->config, 1, &count) && count == 1
  )
    return true;

  const EGLint fallback_attrs[] = {
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES2_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_NONE,
  };
  count = 0;
  if (
    eglChooseConfig(egl->display, fallback_attrs, &egl->config, 1, &count)
    && count == 1
  )
    return true;

  wlr_log(WLR_ERROR, "Failed to choose an EGL ES2 config");
  return false;
}

static bool load_egl_extensions(EglContext* egl) {
  const char* exts = eglQueryString(egl->display, EGL_EXTENSIONS);
  if (!exts) {
    wlr_log(WLR_ERROR, "Failed to query EGL display extensions");
    return false;
  }

  egl->has_image_base = egl_has_extension(exts, "EGL_KHR_image_base");
  egl->has_dmabuf_import =
    egl_has_extension(exts, "EGL_EXT_image_dma_buf_import");
  egl->has_dmabuf_modifiers =
    egl_has_extension(exts, "EGL_EXT_image_dma_buf_import_modifiers");
  egl->has_surfaceless_context =
    egl_has_extension(exts, "EGL_KHR_surfaceless_context");
  egl->has_native_fence =
    egl_has_extension(exts, "EGL_ANDROID_native_fence_sync");
  egl->has_fence_sync = egl_has_extension(exts, "EGL_KHR_fence_sync");
  egl->has_wait_sync = egl_has_extension(exts, "EGL_KHR_wait_sync");

  if (egl->has_image_base) {
    egl->create_image = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
      eglGetProcAddress("eglCreateImageKHR")
    );
    egl->destroy_image = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
      eglGetProcAddress("eglDestroyImageKHR")
    );
    if (!egl->create_image || !egl->destroy_image)
      egl->has_image_base = false;
  }

  if (egl->has_dmabuf_modifiers) {
    egl->query_dmabuf_formats =
      reinterpret_cast<PFNEGLQUERYDMABUFFORMATSEXTPROC>(
        eglGetProcAddress("eglQueryDmaBufFormatsEXT")
      );
    egl->query_dmabuf_modifiers =
      reinterpret_cast<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(
        eglGetProcAddress("eglQueryDmaBufModifiersEXT")
      );
    if (!egl->query_dmabuf_formats || !egl->query_dmabuf_modifiers)
      egl->has_dmabuf_modifiers = false;
  }

  if (egl->has_fence_sync) {
    egl->create_sync = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(
      eglGetProcAddress("eglCreateSyncKHR")
    );
    egl->destroy_sync = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(
      eglGetProcAddress("eglDestroySyncKHR")
    );
    egl->client_wait_sync = reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(
      eglGetProcAddress("eglClientWaitSyncKHR")
    );
    if (!egl->create_sync || !egl->destroy_sync)
      egl->has_fence_sync = false;
  }

  if (egl->has_wait_sync) {
    egl->wait_sync = reinterpret_cast<PFNEGLWAITSYNCKHRPROC>(
      eglGetProcAddress("eglWaitSyncKHR")
    );
    if (!egl->wait_sync)
      egl->has_wait_sync = false;
  }

  if (egl->has_native_fence) {
    egl->dup_native_fence_fd =
      reinterpret_cast<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>(
        eglGetProcAddress("eglDupNativeFenceFDANDROID")
      );
    if (!egl->dup_native_fence_fd || !egl->has_fence_sync)
      egl->has_native_fence = false;
  }

  egl->image_target_texture =
    reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
      eglGetProcAddress("glEGLImageTargetTexture2DOES")
    );
  egl->image_target_renderbuffer =
    reinterpret_cast<PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC>(
      eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES")
    );

  if (!egl->image_target_texture || !egl->image_target_renderbuffer) {
    egl->has_dmabuf_import = false;
    egl->has_dmabuf_modifiers = false;
  }

  const char* gl_exts =
    reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  egl->has_bgra = egl_has_extension(gl_exts, "GL_EXT_texture_format_BGRA8888");
  egl->has_egl_image_external =
    egl_has_extension(gl_exts, "GL_OES_EGL_image_external")
    && egl->image_target_texture;

  if (!egl->has_bgra) {
    wlr_log(
      WLR_ERROR,
      "GL_EXT_texture_format_BGRA8888 is required for wl_shm ARGB8888"
    );
    return false;
  }

  return true;
}

static bool try_egl_display(
  EglContext* egl,
  EGLDisplay display,
  bool surfaceless,
  const char* backend_name,
  EGLint* major,
  EGLint* minor
) {
  if (display == EGL_NO_DISPLAY)
    return false;

  EGLint local_major = 0, local_minor = 0;
  if (!eglInitialize(display, &local_major, &local_minor)) {
    wlr_log(
      WLR_DEBUG,
      "EGL display (%s) failed to initialize: 0x%x",
      backend_name,
      eglGetError()
    );
    return false;
  }

  egl->display = display;
  egl->surfaceless = surfaceless;
  egl->display_backend = backend_name;
  *major = local_major;
  *minor = local_minor;
  return true;
}

static void drop_gbm_for_surfaceless(EglContext* egl) {
  if (egl->gbm_device) {
    gbm_device_destroy(egl->gbm_device);
    egl->gbm_device = nullptr;
    egl->owns_gbm_device = false;
  }
  // Do not advertise a DRM device that this surfaceless context is not
  // necessarily using.
  if (egl->drm_fd >= 0 && egl->owns_drm_fd)
    close(egl->drm_fd);
  egl->drm_fd = -1;
  egl->owns_drm_fd = false;
}

bool egl_context_init(EglContext* egl, wlr_backend* backend) {
  memset(egl, 0, sizeof(*egl));
  egl->display = EGL_NO_DISPLAY;
  egl->context = EGL_NO_CONTEXT;
  egl->surface = EGL_NO_SURFACE;
  egl->drm_fd = -1;

  resolve_drm_fd(egl, backend);

  const char* client_exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
  if (!client_exts) {
    wlr_log(WLR_ERROR, "EGL_EXT_client_extensions not supported");
    return false;
  }

  auto get_platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
    eglGetProcAddress("eglGetPlatformDisplayEXT")
  );

  if (egl->drm_fd >= 0) {
    egl->gbm_device = gbm_create_device(egl->drm_fd);
    if (!egl->gbm_device) {
      wlr_log(WLR_ERROR, "Failed to create GBM device");
      return false;
    }
    egl->owns_gbm_device = true;
  }

  EGLint major = 0, minor = 0;
  bool initialized = false;

  if (!initialized && egl->gbm_device) {
    EGLDisplay display = EGL_NO_DISPLAY;
    if (get_platform_display) {
      display =
        get_platform_display(EGL_PLATFORM_GBM_KHR, egl->gbm_device, nullptr);
    } else {
      display =
        eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(egl->gbm_device));
    }
    initialized = try_egl_display(egl, display, false, "gbm", &major, &minor);
  }

  if (!initialized) {
    EGLDisplay display = EGL_NO_DISPLAY;
    if (get_platform_display) {
      display = get_platform_display(
        EGL_PLATFORM_SURFACELESS_MESA,
        EGL_DEFAULT_DISPLAY,
        nullptr
      );
    } else {
      display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    initialized =
      try_egl_display(egl, display, true, "surfaceless", &major, &minor);
  }

  if (!initialized || egl->display == EGL_NO_DISPLAY) {
    wlr_log(WLR_ERROR, "Failed to create or initialize EGL display");
    return false;
  }

  if (egl->surfaceless)
    drop_gbm_for_surfaceless(egl);

  wlr_log(
    WLR_INFO,
    "Initialized EGL %d.%d for raylib renderer (%s)",
    major,
    minor,
    egl->display_backend ? egl->display_backend : "unknown"
  );

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    wlr_log(WLR_ERROR, "Failed to bind EGL OpenGL ES API");
    return false;
  }

  if (!choose_config(egl))
    return false;

  const EGLint context_attrs[] = {
    EGL_CONTEXT_CLIENT_VERSION,
    2,
    EGL_NONE,
  };
  egl->context =
    eglCreateContext(egl->display, egl->config, EGL_NO_CONTEXT, context_attrs);
  if (egl->context == EGL_NO_CONTEXT) {
    wlr_log(WLR_ERROR, "Failed to create EGL ES2 context");
    return false;
  }

  EglSavedContext saved {};
  if (!egl_make_current(egl, &saved)) {
    wlr_log(WLR_ERROR, "Failed to make EGL context current");
    return false;
  }

  if (!load_egl_extensions(egl)) {
    egl_restore_context(&saved);
    return false;
  }
  egl_restore_context(&saved);

  return true;
}

void egl_context_finish(EglContext* egl) {
  if (egl->display != EGL_NO_DISPLAY) {
    eglMakeCurrent(
      egl->display,
      EGL_NO_SURFACE,
      EGL_NO_SURFACE,
      EGL_NO_CONTEXT
    );
    if (egl->surface != EGL_NO_SURFACE)
      eglDestroySurface(egl->display, egl->surface);
    if (egl->context != EGL_NO_CONTEXT)
      eglDestroyContext(egl->display, egl->context);
    eglTerminate(egl->display);
  }

  if (egl->gbm_device)
    gbm_device_destroy(egl->gbm_device);
  if (egl->drm_fd >= 0 && egl->owns_drm_fd)
    close(egl->drm_fd);

  egl->display = EGL_NO_DISPLAY;
  egl->context = EGL_NO_CONTEXT;
  egl->surface = EGL_NO_SURFACE;
  egl->gbm_device = nullptr;
  egl->drm_fd = -1;
}

bool egl_make_current(EglContext* egl, EglSavedContext* saved) {
  saved->display = eglGetCurrentDisplay();
  saved->context = eglGetCurrentContext();
  saved->draw_surface = eglGetCurrentSurface(EGL_DRAW);
  saved->read_surface = eglGetCurrentSurface(EGL_READ);

  if (!egl->surfaceless && egl->surface == EGL_NO_SURFACE) {
    const EGLint attrs[] = {
      EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
    };
    egl->surface = eglCreatePbufferSurface(egl->display, egl->config, attrs);
    if (egl->surface == EGL_NO_SURFACE) {
      wlr_log(WLR_ERROR, "Failed to create EGL pbuffer");
      return false;
    }
  }

  EGLSurface draw = egl->surfaceless ? EGL_NO_SURFACE : egl->surface;
  EGLSurface read = draw;
  if (egl->surfaceless && !egl->has_surfaceless_context) {
    if (egl->surface == EGL_NO_SURFACE) {
      const EGLint attrs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
      };
      egl->surface = eglCreatePbufferSurface(egl->display, egl->config, attrs);
      if (egl->surface == EGL_NO_SURFACE) {
        wlr_log(WLR_ERROR, "Failed to create EGL pbuffer");
        return false;
      }
    }
    draw = egl->surface;
    read = egl->surface;
  }

  if (!eglMakeCurrent(egl->display, draw, read, egl->context)) {
    wlr_log(WLR_ERROR, "eglMakeCurrent failed: 0x%x", eglGetError());
    return false;
  }
  return true;
}

void egl_restore_context(const EglSavedContext* saved) {
  if (saved->display == EGL_NO_DISPLAY) {
    eglMakeCurrent(
      EGL_NO_DISPLAY,
      EGL_NO_SURFACE,
      EGL_NO_SURFACE,
      EGL_NO_CONTEXT
    );
    return;
  }
  eglMakeCurrent(
    saved->display,
    saved->draw_surface,
    saved->read_surface,
    saved->context
  );
}

} // namespace ura::render::raylib
