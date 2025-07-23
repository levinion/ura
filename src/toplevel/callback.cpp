#include "ura/toplevel.hpp"
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {
// create a new toplevel
void on_new_toplevel(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);
  auto toplevel = new UraToplevel {};
  toplevel->init(xdg_toplevel);
}

void on_toplevel_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  if (!toplevel->mapped)
    toplevel->map();
  toplevel->focus();
}

void on_toplevel_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->unmap();
}

void on_toplevel_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->commit();
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->destroy();
  delete toplevel;
}

// TODO: handle floating toplevels

// void on_toplevel_request_move(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   toplevel->move();
// }
//
// void on_toplevel_request_resize(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
//   toplevel->resize(event->edges);
// }

// void on_toplevel_request_maximize(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   // equal to fullscreen
//   toplevel->toggle_fullscreen();
// }

void on_toplevel_request_fullscreen(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->toggle_fullscreen();
}
} // namespace ura
