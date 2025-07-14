#include "ura/server.hpp"
#include "ura/popup.hpp"
#include "ura/callback.hpp"

namespace ura {

void on_new_popup(wl_listener* listener, void* data) {
  auto xdg_popup = static_cast<wlr_xdg_popup*>(data);
  auto popup = new UraPopup {};
  popup->xdg_popup = xdg_popup;

  auto parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
  auto parent_tree = static_cast<wlr_scene_tree*>(parent->data);
  xdg_popup->base->data =
    wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

  // callbacks
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &xdg_popup->base->surface->events.commit,
    on_popup_commit,
    popup
  );

  server->runtime->register_callback(
    &xdg_popup->base->surface->events.destroy,
    on_popup_destroy,
    popup
  );
}

void on_popup_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);

  if (popup->xdg_popup->base->initial_commit) {
    // TODO: do some work
    wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
  }
}

void on_popup_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);
  server->runtime->remove(popup);
}

} // namespace ura
