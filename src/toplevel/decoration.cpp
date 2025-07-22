#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"

namespace ura {

void on_new_toplevel_decoration(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel_decoration = static_cast<wlr_xdg_toplevel_decoration_v1*>(data);
  auto toplevel =
    UraToplevel::from(toplevel_decoration->toplevel->base->surface);
  toplevel->decoration = toplevel_decoration;
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
  // server->runtime->register_callback(
  //   &server_decoration->events.mode,
  //   on_server_decoration_mode,
  //   nullptr
  // );
}

void on_server_decoration_mode(wl_listener* listener, void* data) {
  // TODO: impl this
}

} // namespace ura
