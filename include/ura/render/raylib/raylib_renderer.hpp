#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct wlr_backend;
struct wlr_renderer;

struct wlr_renderer* ura_raylib_renderer_create(struct wlr_backend* backend);

#ifdef __cplusplus
}
#endif
