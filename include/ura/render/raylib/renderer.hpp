#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>
#include <pixman.h>
#include <rlgl.h>
#include <ctime>
#include <cstdint>

extern "C" {
#include <wayland-server-core.h>
}

#define static
#define class class_
#define namespace namespace_
#define delete delete_
extern "C" {
#include <wlr/backend.h>
#include <wlr/render/dmabuf.h>
#include <wlr/render/drm_format_set.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/interface.h>
#include <wlr/render/pass.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/util/addon.h>
}
#undef static
#undef class
#undef namespace
#undef delete

struct gbm_device;

namespace ura::render::raylib {

struct Renderer;
struct RenderPass;
struct Texture;
struct Buffer;
struct RenderTimer;

struct GlFormat {
  uint32_t drm_format;
  GLint internal_format;
  GLenum format;
  GLenum type;
  bool has_alpha;
  // Byte indices (R,G,B,A) used to convert DRM memory order to GL_RGBA.
  uint8_t upload_swizzle[4];
};

struct Shader {
  GLuint id;
  int locs[RL_MAX_SHADER_LOCATIONS];
  int mvp;
  int texture;
  int color;
  int pos_attrib;
  int tex_attrib;
  int tint;
  bool valid;
};

struct EglSavedContext {
  EGLDisplay display;
  EGLContext context;
  EGLSurface draw_surface;
  EGLSurface read_surface;
};

struct EglContext {
  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
  EGLConfig config;
  struct gbm_device* gbm_device;
  int drm_fd;
  bool owns_drm_fd;
  bool owns_gbm_device;
  bool surfaceless;
  const char* display_backend;

  bool has_image_base;
  bool has_dmabuf_import;
  bool has_dmabuf_modifiers;
  bool has_surfaceless_context;
  bool has_native_fence;
  bool has_fence_sync;
  bool has_wait_sync;
  bool has_bgra;
  bool has_egl_image_external;

  PFNEGLCREATEIMAGEKHRPROC create_image;
  PFNEGLDESTROYIMAGEKHRPROC destroy_image;
  PFNEGLQUERYDMABUFFORMATSEXTPROC query_dmabuf_formats;
  PFNEGLQUERYDMABUFMODIFIERSEXTPROC query_dmabuf_modifiers;
  PFNEGLCREATESYNCKHRPROC create_sync;
  PFNEGLDESTROYSYNCKHRPROC destroy_sync;
  PFNEGLCLIENTWAITSYNCKHRPROC client_wait_sync;
  PFNEGLWAITSYNCKHRPROC wait_sync;
  PFNEGLDUPNATIVEFENCEFDANDROIDPROC dup_native_fence_fd;
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture;
  PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC image_target_renderbuffer;

  wlr_drm_format_set texture_formats;
  wlr_drm_format_set render_formats;
  wlr_drm_format_set shm_formats;
};

struct Renderer {
  wlr_renderer base;
  EglContext egl;
  int drm_fd;
  Shader rect;
  Shader tex_rgba;
  Shader tex_rgbx;
  Shader external;
  Shader effect;
  GLuint white_texture;
  bool rlgl_ready;
  wl_list buffers; // Buffer::link
  wl_list textures; // Texture::link
};

struct Buffer {
  Renderer* renderer;
  wlr_buffer* buffer;
  EGLImageKHR image;
  bool external_only;
  GLuint rbo;
  GLuint fbo;
  int width;
  int height;
  uint32_t format;
  wlr_addon addon;
  wl_list link; // Renderer::buffers
};

struct Texture {
  wlr_texture base;
  Renderer* renderer;
  GLuint tex;
  GLenum target;
  bool has_alpha;
  bool mutable_texture;
  uint32_t drm_format;
  Buffer* buffer;
  wl_list link; // Renderer::textures
};

struct RenderPass {
  wlr_render_pass base;
  Renderer* renderer;
  wlr_buffer* buffer;
  Buffer* target;
  GLuint fbo;
  GLuint shm_texture;
  bool shm_access;
  bool shm_writable;
  void* shm_data;
  size_t shm_stride;
  uint32_t shm_format;
  int width;
  int height;
  pixman_region32_t damage;
  wlr_drm_syncobj_timeline* signal_timeline;
  uint64_t signal_point;
  RenderTimer* timer;
  EglSavedContext saved;
  bool rlgl_active;
};

struct RenderTimer {
  wlr_render_timer base;
  timespec start;
  timespec end;
  bool started;
};

// egl.cpp
bool egl_context_init(EglContext* egl, wlr_backend* backend);
void egl_context_finish(EglContext* egl);
bool egl_make_current(EglContext* egl, EglSavedContext* saved);
void egl_restore_context(const EglSavedContext* saved);
bool egl_has_extension(const char* extensions, const char* name);

// resources.cpp
const GlFormat* gl_format_from_drm(uint32_t format);
const GlFormat* gl_format_from_gl(GLenum format, GLenum type, bool alpha);
bool gl_format_has_alpha(uint32_t format);
bool gl_format_is_ycbcr(uint32_t format);
bool renderer_init_surface_formats(Renderer* renderer);
void renderer_finish_surface_formats(Renderer* renderer);

// shader.cpp
bool shader_init(Renderer* renderer);
void shader_finish(Renderer* renderer);
Matrix shader_ortho_matrix(int width, int height);
void shader_set_projection(int location, int width, int height);
// buffer.cpp
Buffer* buffer_get(Renderer* renderer, wlr_buffer* buffer);
void buffer_finish(Buffer* buffer);
bool buffer_ensure_render_target(Buffer* buffer);

// texture.cpp
Texture* texture_from_buffer(Renderer* renderer, wlr_buffer* buffer);
void texture_destroy(Texture* texture);

// render_pass.cpp
RenderPass* render_pass_begin(
  Renderer* renderer,
  wlr_buffer* buffer,
  const wlr_buffer_pass_options* options
);
bool render_pass_submit(RenderPass* pass);
void render_pass_add_texture(
  RenderPass* pass,
  const wlr_render_texture_options* options
);
void render_pass_add_rect(
  RenderPass* pass,
  const wlr_render_rect_options* options
);

// effects.cpp
void effects_apply(Renderer* renderer, RenderPass* pass);

} // namespace ura::render::raylib
