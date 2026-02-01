#pragma once
#include "ura-ipc-protocol.h"

namespace ura {

struct ura_ipc_request_event {
  struct wl_resource* resource;
  const char* script;
};

struct ura_ipc {
  struct wl_global* global;

  struct {
    struct wl_signal request;
    struct wl_signal destroy;
  } events;

  void* data;

  struct {
    struct wl_listener display_destroy;
  } WLR_PRIVATE;
};

ura_ipc* ura_ipc_create(wl_display* display);
} // namespace ura
