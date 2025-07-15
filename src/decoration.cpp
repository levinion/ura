#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "wlr/types/wlr_xdg_decoration_v1.h"
#include "ura/decoration.hpp"

namespace ura {

void on_new_toplevel_decoration(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration = static_cast<wlr_xdg_toplevel_decoration_v1*>(data);
  auto decoration = new UraDecoration {};
  decoration->toplevel_decoration = toplevel_decoration;

  server->runtime->register_callback(
    &toplevel_decoration->events.destroy,
    on_toplevel_decoration_destroy,
    decoration
  );

  server->runtime->register_callback(
    &toplevel_decoration->events.request_mode,
    on_toplevel_decoration_request_mode,
    decoration
  );

  // TODO: find out why this will lead to crash

  // decoration->set_decoration(WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE);
}

void on_toplevel_decoration_request_mode(
  struct wl_listener* listener,
  void* data
) {
  auto server = UraServer::get_instance();
  auto decoration = server->runtime->fetch<UraDecoration*>(listener);

  // decoration->set_decoration(WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE);
}

void on_toplevel_decoration_destroy(struct wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto decoration = server->runtime->fetch<UraDecoration*>(listener);
  server->runtime->remove(decoration);
  delete decoration;
}
} // namespace ura
