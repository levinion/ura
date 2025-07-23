#include "ura/client.hpp"
#include <utility>

namespace ura {

UraSurfaceType get_surface_type(wlr_surface* surface) {
  if (wlr_xdg_toplevel_try_from_wlr_surface(surface))
    return UraSurfaceType::Toplevel;
  if (wlr_layer_surface_v1_try_from_wlr_surface(surface))
    return UraSurfaceType::LayerShell;
  if (wlr_xdg_popup_try_from_wlr_surface(surface))
    return UraSurfaceType::Popup;
  return UraSurfaceType::Unknown;
}

void UraClient::focus() {
  switch (this->type) {
    case UraSurfaceType::Toplevel:
      this->transform<UraToplevel>()->focus();
      break;
    case ura::UraSurfaceType::LayerShell:
      this->transform<UraLayerShell>()->focus();
      break;
    case ura::UraSurfaceType::Popup:
      this->transform<UraPopup>()->focus();
      break;
    default:
      std::unreachable();
  }
}
} // namespace ura
