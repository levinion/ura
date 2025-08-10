#include "ura/toplevel.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {
// toplevel/foreign.cpp
void on_foreign_toplevel_handle_request_activate(
  wl_listener* listener,
  void* data
) {
  auto event =
    static_cast<wlr_foreign_toplevel_handle_v1_activated_event*>(data);
  auto server = UraServer::get_instance();
  auto toplevel = static_cast<UraToplevel*>(event->toplevel->data);
  toplevel->activate();
}

void on_foreign_toplevel_handle_request_fullscreen(
  wl_listener* listener,
  void* data
) {
  auto event =
    static_cast<wlr_foreign_toplevel_handle_v1_fullscreen_event*>(data);
  auto server = UraServer::get_instance();
  auto toplevel = static_cast<UraToplevel*>(event->toplevel->data);
  toplevel->layout != "fullscreen" ? toplevel->set_layout("fullscreen")
                                   : toplevel->recover_layout();
}
} // namespace ura
