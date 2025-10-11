#include "ura/view/toplevel.hpp"
#include "ura/view/view.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"

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
  auto output = server->view->current_output();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->destroy();
  delete toplevel;
}

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
  toplevel->layout != "fullscreen"
    ? toplevel->set_layout("fullscreen")
    : toplevel->set_layout(toplevel->last_layout.value_or("tiling"));
}

void on_toplevel_set_app_id(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->set_app_id(toplevel->app_id());
}

void on_toplevel_set_title(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->set_title(toplevel->title());
}
} // namespace ura
