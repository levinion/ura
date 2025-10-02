#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/toplevel.hpp"

namespace ura {

void on_new_toplevel_decoration(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration = static_cast<wlr_xdg_toplevel_decoration_v1*>(data);
  auto toplevel =
    UraToplevel::from(toplevel_decoration->toplevel->base->surface);
  toplevel->decoration = toplevel_decoration;

  server->runtime->register_callback(
    &toplevel_decoration->events.request_mode,
    on_toplevel_decoration_request_mode,
    toplevel_decoration
  );
  server->runtime->register_callback(
    &toplevel_decoration->events.destroy,
    on_toplevel_decoration_destroy,
    toplevel_decoration
  );

  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      toplevel_decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }
}

void on_toplevel_decoration_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration =
    server->runtime->fetch<wlr_xdg_toplevel_decoration_v1*>(listener);
  server->runtime->remove(toplevel_decoration);
}

void on_toplevel_decoration_request_mode(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration =
    server->runtime->fetch<wlr_xdg_toplevel_decoration_v1*>(listener);
  auto toplevel =
    UraToplevel::from(toplevel_decoration->toplevel->base->surface);
  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      toplevel_decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }
}

void on_new_server_decoration(wl_listener* listener, void* data) {
  auto server_decoration = static_cast<wlr_server_decoration*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &server_decoration->events.mode,
    on_server_decoration_mode,
    server_decoration
  );
  server->runtime->register_callback(
    &server_decoration->events.destroy,
    on_server_decoration_destroy,
    server_decoration
  );
}

void on_server_decoration_mode(wl_listener* listener, void* data) {
  // DO NOTHING
}

void on_server_decoration_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto server_decoration =
    server->runtime->fetch<wlr_server_decoration*>(listener);
  server->runtime->remove(server_decoration);
}

} // namespace ura
