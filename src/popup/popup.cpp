#include "ura/popup.hpp"
#include <utility>
#include "ura/layer_shell.hpp"
#include "ura/toplevel.hpp"
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"
#include "ura/client.hpp"

namespace ura {

void UraPopup::init(wlr_xdg_popup* xdg_popup) {
  auto server = UraServer::get_instance();
  this->xdg_popup = xdg_popup;
  this->xdg_popup->base->surface->data = this;

  if (xdg_popup->parent) {
    auto parent = xdg_popup->parent;
    if (!parent) {
      delete this;
      return;
    }
    auto client = UraClient::from(xdg_popup->parent);
    switch (client.type) {
      case UraSurfaceType::Toplevel: {
        auto toplevel = client.transform<UraToplevel>();
        auto parent_tree = toplevel->scene_tree;
        if (!parent_tree) {
          delete this;
          return;
        }
        xdg_popup->base->data =
          wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);
        break;
      }
      case UraSurfaceType::LayerShell: {
        auto layer_shell = client.transform<UraLayerShell>();
        auto parent_tree = layer_shell->scene_tree;
        if (!parent_tree) {
          delete this;
          return;
        }
        xdg_popup->base->data =
          wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);
        break;
      }
      default:
        std::unreachable();
    }
  } else {
    // this happens when a popup has no parent
    // then send them to current output's popup layer
    auto output = server->current_output();
    xdg_popup->base->data =
      wlr_scene_xdg_surface_create(output->popup, xdg_popup->base);
    output->popups.push_back(this);
  }

  // callbacks
  server->runtime->register_callback(
    &xdg_popup->base->surface->events.commit,
    on_popup_commit,
    this
  );
  server->runtime->register_callback(
    &xdg_popup->base->surface->events.destroy,
    on_popup_destroy,
    this
  );
}

UraPopup* UraPopup::from(wlr_surface* surface) {
  return static_cast<UraPopup*>(surface->data);
}

void UraPopup::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat;
  auto keyboard = wlr_seat_get_keyboard(seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      seat,
      this->xdg_popup->base->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}
} // namespace ura
