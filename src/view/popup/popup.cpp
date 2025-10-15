#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/view/popup.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/output.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"

namespace ura {

bool UraPopup::init(wlr_xdg_popup* xdg_popup) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  if (xdg_popup->parent) {
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
      case UraSurfaceType::Popup: {
        auto popup = client.transform<UraPopup>();
        auto parent_tree = popup->scene_tree;
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
    this->scene_tree = wlr_scene_xdg_surface_create(
      server->view->get_scene_tree_or_create(UraSceneLayer::Popup),
      xdg_popup->base
    );
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

  server->globals.insert(this->id());

  return true;
}

UraPopup* UraPopup::from(wlr_surface* surface) {
  return static_cast<UraPopup*>(surface->data);
}

UraPopup* UraPopup::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    return reinterpret_cast<UraPopup*>(id);
  return nullptr;
}

void UraPopup::commit() {
  if (!this->xdg_popup->base->initial_commit)
    return;

  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  auto box = output->logical_geometry().to_wlr_box();

  if (!this->xdg_popup->parent) {
    wlr_xdg_popup_unconstrain_from_box(this->xdg_popup, &box);
    return;
  }

  auto client = UraClient::from(this->xdg_popup->parent);
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
    case UraSurfaceType::Popup: {
      auto popup = client.transform<UraPopup>();
      int lx, ly;
      wlr_scene_node_coords(&popup->scene_tree->node, &lx, &ly);
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
  auto output = server->view->current_output();
  output->popups.remove(this);
  server->runtime->remove(this);
  wlr_xdg_popup *_popup, *tmp;
  wl_list_for_each_safe(_popup, tmp, &this->xdg_popup->base->popups, link) {
    wlr_xdg_popup_destroy(_popup);
  }
  server->globals.erase(this->id());
}

uint64_t UraPopup::id() {
  return reinterpret_cast<uint64_t>(this);
}

} // namespace ura
