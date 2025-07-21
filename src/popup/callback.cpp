#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/popup.hpp"
#include "ura/callback.hpp"
#include "ura/runtime.hpp"
#include "ura/toplevel.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"

namespace ura {
void on_new_popup(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto xdg_popup = static_cast<wlr_xdg_popup*>(data);
  auto popup = new UraPopup {};
  popup->xdg_popup = xdg_popup;

  if (xdg_popup->parent) {
    auto parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    if (!parent) {
      delete popup;
      return;
    }
    auto parent_tree = static_cast<wlr_scene_tree*>(parent->data);
    if (!parent_tree) {
      delete popup;
      return;
    }
    xdg_popup->base->data =
      wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);
  } else {
    // this happens when a popup has no parent
    // then send them to current output's popup layer
    auto output = server->current_output();
    xdg_popup->base->data =
      wlr_scene_xdg_surface_create(output->popup, xdg_popup->base);
    output->popups.push_back(popup);
  }

  // callbacks
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
  auto output = server->current_output();
  if (popup->xdg_popup && popup->xdg_popup->base->initial_commit) {
    // try parse popup's parent as a toplevel
    auto _toplevel =
      wlr_xdg_toplevel_try_from_wlr_surface(popup->xdg_popup->parent);
    if (_toplevel) {
      auto toplevel = UraToplevel::from(_toplevel);
      int lx, ly;
      wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);
      wlr_box box;
      wlr_output_effective_resolution(output->output, &box.width, &box.height);
      box.x = -lx;
      box.y = -ly;
      wlr_xdg_popup_unconstrain_from_box(popup->xdg_popup, &box);
      return;
    }

    // try parse popup's parent as a layer_shell
    auto _layer_shell =
      wlr_layer_surface_v1_try_from_wlr_surface(popup->xdg_popup->parent);
    if (_layer_shell) {
      auto layer_shell = UraLayerShell::from(_layer_shell);
      int lx, ly;
      wlr_scene_node_coords(&layer_shell->scene_tree->node, &lx, &ly);
      wlr_box box;
      wlr_output_effective_resolution(output->output, &box.width, &box.height);
      box.x = -lx;
      box.y = -ly;
      wlr_xdg_popup_unconstrain_from_box(popup->xdg_popup, &box);
      return;
    }
  }
}

void on_popup_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);
  server->runtime->remove(popup);

  auto output = server->current_output();
  // try delete the popup if exists
  output->popups.remove(popup);
  delete popup;
}
} // namespace ura
