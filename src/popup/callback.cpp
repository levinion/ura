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
  popup->init(xdg_popup);
}

void on_popup_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto popup = server->runtime->fetch<UraPopup*>(listener);

  // TODO: obs popup lead to crash when executing this line
  if (!popup->xdg_popup || !popup->xdg_popup->base
      || !popup->xdg_popup->base->initial_commit || !popup->xdg_popup->parent)
    return;

  // TODO: focus popup when popup shows

  // try parse popup's parent as a toplevel
  auto _toplevel =
    wlr_xdg_toplevel_try_from_wlr_surface(popup->xdg_popup->parent);
  if (_toplevel) {
    auto toplevel = UraToplevel::from(_toplevel->base->surface);
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
    auto layer_shell = UraLayerShell::from(_layer_shell->surface);
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

void on_popup_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto popup = server->runtime->fetch<UraPopup*>(listener);
  auto output = server->current_output();
  // try delete the popup if exists
  output->popups.remove(popup);
  server->runtime->remove(popup);
  delete popup;
}
} // namespace ura
