#include "ura/popup.hpp"
#include "ura/layer_shell.hpp"
#include "ura/toplevel.hpp"
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"
#include "ura/client.hpp"

namespace ura {

bool UraPopup::init(wlr_xdg_popup* xdg_popup) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  if (xdg_popup->parent) {
    auto parent = xdg_popup->parent;
    auto client = UraClient::from(xdg_popup->parent);
    switch (client.type) {
      case UraSurfaceType::Toplevel: {
        auto toplevel = client.transform<UraToplevel>();
        auto parent_tree = toplevel->scene_tree;
        if (!parent_tree) {
          return false;
        }
        this->scene_tree =
          wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);
        break;
      }
      case UraSurfaceType::LayerShell: {
        auto layer_shell = client.transform<UraLayerShell>();
        auto parent_tree = layer_shell->scene_tree;
        if (!parent_tree) {
          return false;
        }
        this->scene_tree =
          wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);
        break;
      }
      default:
        return false;
    }
  } else {
    // this happens when a popup has no parent
    // then send them to current output's popup layer
    this->scene_tree =
      wlr_scene_xdg_surface_create(output->popup, xdg_popup->base);
  }

  output->popups.push_back(this);

  this->xdg_popup = xdg_popup;
  this->xdg_popup->base->surface->data = this;

  // callbacks
  server->runtime->register_callback(
    &xdg_popup->base->surface->events.commit,
    on_popup_commit,
    this
  );
  server->runtime
    ->register_callback(&xdg_popup->events.destroy, on_popup_destroy, this);
  return true;
}

UraPopup* UraPopup::from(wlr_surface* surface) {
  return static_cast<UraPopup*>(surface->data);
}

void UraPopup::commit() {
  if (!this->xdg_popup || !this->xdg_popup->base || !this->xdg_popup->parent)
    return;

  if (!this->xdg_popup->base->initial_commit)
    return;

  auto server = UraServer::get_instance();
  auto output = server->current_output();

  auto client = UraClient::from(this->xdg_popup->parent);
  auto box = output->logical_geometry();
  switch (client.type) {
    case UraSurfaceType::Toplevel: {
      // try parse popup's parent as a toplevel
      auto toplevel = client.transform<UraToplevel>();
      int lx, ly;
      wlr_scene_node_coords(&toplevel->scene_tree->node, &lx, &ly);
      box.x = -lx;
      box.y = -ly;
      wlr_xdg_popup_unconstrain_from_box(this->xdg_popup, &box);
      break;
    }
    case UraSurfaceType::LayerShell: {
      // try parse popup's parent as a layer_shell
      auto layer_shell = client.transform<UraLayerShell>();
      int lx, ly;
      wlr_scene_node_coords(&layer_shell->scene_tree->node, &lx, &ly);
      box.x = -lx;
      box.y = -ly;
      wlr_xdg_popup_unconstrain_from_box(this->xdg_popup, &box);
      break;
    }
    default:
      break;
  }
}

void UraPopup::destroy() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  output->popups.remove(this);
  server->runtime->remove(this);
}
} // namespace ura
