#include "ura/ura.hpp"

namespace ura {

enum class UraSurfaceType { Toplevel, LayerShell, Popup, Unknown };

inline UraSurfaceType get_surface_type(wlr_surface* surface) {
  if (wlr_xdg_toplevel_try_from_wlr_surface(surface))
    return UraSurfaceType::Toplevel;
  if (wlr_layer_surface_v1_try_from_wlr_surface(surface))
    return UraSurfaceType::LayerShell;
  if (wlr_xdg_popup_try_from_wlr_surface(surface))
    return UraSurfaceType::Popup;
  return UraSurfaceType::Unknown;
}

} // namespace ura
