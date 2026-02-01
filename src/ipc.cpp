#include "ura/core/ipc.hpp"
#include "ura-ipc-protocol.h"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"

namespace ura {

void ura_ipc_handle_call_method(
  wl_client* client,
  wl_resource* resource,
  const char* script
) {
  auto ipc = (ura_ipc*)wl_resource_get_user_data(resource);

  ura_ipc_request_event event = { .resource = resource, .script = script };

  wl_signal_emit(&ipc->events.request, &event);
}

static const struct ura_ipc_interface ura_ipc_impl = {
  .call = ura_ipc_handle_call_method,
  .destroy =
    [](struct wl_client* c, struct wl_resource* r) { wl_resource_destroy(r); },
};

void ura_ipc_bind(
  struct wl_client* client,
  void* data,
  uint32_t version,
  uint32_t id
) {
  auto ipc = (ura_ipc*)data;

  wl_resource* resource =
    wl_resource_create(client, &ura_ipc_interface, version, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &ura_ipc_impl, ipc, nullptr);
}

void handle_display_destroy(wl_listener* listener, void* data) {
  ura_ipc* ipc = wl_container_of(listener, ipc, WLR_PRIVATE.display_destroy);
  wl_signal_emit(&ipc->events.destroy, ipc);
  wl_global_destroy(ipc->global);
  free(ipc);
}

ura_ipc* ura_ipc_create(wl_display* display) {
  auto ipc = new ura_ipc;
  if (!ipc)
    return nullptr;

  wl_signal_init(&ipc->events.request);
  wl_signal_init(&ipc->events.destroy);

  ipc->global =
    wl_global_create(display, &ura_ipc_interface, 1, ipc, ura_ipc_bind);
  if (!ipc->global) {
    free(ipc);
    return nullptr;
  }

  ipc->WLR_PRIVATE.display_destroy.notify = handle_display_destroy;
  wl_display_add_destroy_listener(display, &ipc->WLR_PRIVATE.display_destroy);

  return ipc;
}

void on_ura_ipc_request(wl_listener* listener, void* data) {
  auto event = static_cast<ura_ipc_request_event*>(data);
  auto server = UraServer::get_instance();
  auto result = server->lua->execute(event->script);
  if (result) {
    ura_ipc_send_reply(event->resource, 0, result.value().c_str());
  } else {
    ura_ipc_send_reply(event->resource, -1, result.error().c_str());
  }
}
} // namespace ura
