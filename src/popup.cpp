#include "ura/popup.hpp"
#include <cassert>
#include "ura/callback.hpp"

namespace ura {

void on_popup_commit(wl_listener* listener, void* data) {
  UraPopup* popup = wl_container_of(listener, popup, commit);

  if (popup->xdg_popup->base->initial_commit) {
    // TODO: do some work
    wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
  }
}

void on_popup_destroy(wl_listener* listener, void* data) {
  UraPopup* popup = wl_container_of(listener, popup, destroy);

  wl_list_remove(&popup->commit.link);
  wl_list_remove(&popup->destroy.link);

  free(popup);
}

void on_new_popup(wl_listener* listener, void* data) {
  auto xdg_popup = static_cast<wlr_xdg_popup*>(data);
  auto popup = new UraPopup {};
  popup->xdg_popup = xdg_popup;

  auto parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
  assert(parent != NULL);
  auto parent_tree = static_cast<wlr_scene_tree*>(parent->data);
  xdg_popup->base->data =
    wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

  // callbacks
  popup->commit.notify = on_popup_commit;
  wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);
  popup->destroy.notify = on_popup_destroy;
  wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);
}
} // namespace ura
