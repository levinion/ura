#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/view/popup.hpp"
#include <memory>
#include "ura/ura.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/output.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"

namespace ura {

bool UraPopup::init(wlr_xdg_popup* xdg_popup) {
  auto server = UraServer::get_instance();
  if (xdg_popup->parent) {
    auto client = UraClient::from(xdg_popup->parent);
    this->parent = std::make_unique<UraClient>(client);
  }

  this->xdg_popup = xdg_popup;
  this->xdg_popup->base->surface->data = this;
  this->entity = server->entities->create_popup(this);

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

UraPopup* UraPopup::from(uint64_t id) {
  return UraServer::get_instance()->entities->popup(id);
}

UraOutput* UraPopup::output() {
  auto tmp = this->xdg_popup->parent;
  while (tmp) {
    auto c = UraClient::from(tmp);
    if (c.type == UraSurfaceType::Toplevel) {
      if (auto o = c.transform<UraToplevel>()->output())
        return o;
      break;
    } else if (c.type == UraSurfaceType::LayerShell) {
      auto ls = c.transform<UraLayerShell>();
      if (ls->layer_surface->output)
        return UraOutput::from(ls->layer_surface->output);
      break;
    } else if (c.type == UraSurfaceType::Popup) {
      tmp = c.transform<UraPopup>()->xdg_popup->parent;
    } else {
      break;
    }
  }
  return UraServer::get_instance()->view->current_output();
}

void UraPopup::unconstrain() {
  auto output = this->output();
  if (!output)
    return;
  auto box = output->logical_geometry().to_wlr_box();

  auto current = this->xdg_popup->parent;

  while (current) {
    auto parent = UraClient::from(current);
    switch (parent.type) {
      case UraSurfaceType::Toplevel: {
        // try parse popup's parent as a toplevel
        auto toplevel = parent.transform<UraToplevel>();
        auto transform = toplevel->transform();
        box.x -= transform.x;
        box.y -= transform.y;
        break;
      }
      case UraSurfaceType::LayerShell: {
        // try parse popup's parent as a layer_shell
        auto layer_shell = parent.transform<UraLayerShell>();
        auto transform = layer_shell->transform();
        box.x -= transform.x;
        box.y -= transform.y;
        break;
      }
      case UraSurfaceType::Popup: {
        auto popup = parent.transform<UraPopup>();
        current = popup->xdg_popup->parent;
        break;
      }
      default:
        break;
    }
    if (parent.type != UraSurfaceType::Popup)
      break;
  }

  wlr_xdg_popup_unconstrain_from_box(this->xdg_popup, &box);
}

void UraPopup::commit() {
  this->unconstrain();
}

void UraPopup::destroy() {
  auto server = UraServer::get_instance();
  server->runtime->remove(this);
  wlr_xdg_popup *_popup, *tmp;
  wl_list_for_each_safe(_popup, tmp, &this->xdg_popup->base->popups, link) {
    wlr_xdg_popup_destroy(_popup);
  }
  server->entities->destroy(this->entity);
  this->entity = entt::null;
  server->view->schedule_frames();
}

uint64_t UraPopup::id() {
  return UraEntityRegistry::to_id(this->entity);
}

void UraPopup::unfocus() {
  if (!this->parent)
    return;
  auto parent = this->parent.get();
  if (parent->type == UraSurfaceType::Toplevel)
    parent->transform<UraToplevel>()->unfocus();
  else if (parent->type == UraSurfaceType::LayerShell)
    parent->transform<UraLayerShell>()->unfocus();
  else if (parent->type == UraSurfaceType::Popup)
    parent->transform<UraPopup>()->unfocus();
}

} // namespace ura
