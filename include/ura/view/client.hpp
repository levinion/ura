#pragma once

#include <utility>
#include "ura/view/layer_shell.hpp"
#include "ura/view/popup.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/session_lock.hpp"

namespace ura {
enum class UraSurfaceType {
  Toplevel,
  LayerShell,
  Popup,
  SessionLock,
  Unknown,
  Null
};

UraSurfaceType get_surface_type(wlr_surface* surface);

class UraClient {
public:
  UraSurfaceType type;
  wlr_surface* surface;
  std::optional<double> sx; // offset from cursor to left-up corner
  std::optional<double> sy;

  template<typename T>
  T* transform() {
    if constexpr (std::is_same_v<T, UraToplevel>) {
      return UraToplevel::from(this->surface);
    } else if constexpr (std::is_same_v<T, UraLayerShell>) {
      return UraLayerShell::from(this->surface);
    } else if constexpr (std::is_same_v<T, UraPopup>) {
      return UraPopup::from(this->surface);
    } else if constexpr (std::is_same_v<T, UraSessionLockSurface>) {
      return UraSessionLockSurface::from(this->surface);
    }
    std::unreachable();
  }

  template<typename T>
  static UraClient from(T t) {
    auto client = UraClient();
    if constexpr (std::is_same_v<T, wlr_surface*>) {
      client.type = get_surface_type(t);
      client.surface = t;
    } else if constexpr (std::is_same_v<T, UraToplevel*>) {
      client.surface =
        static_cast<UraToplevel*>(t)->xdg_toplevel->base->surface;
      client.type = UraSurfaceType::Toplevel;
    } else if constexpr (std::is_same_v<T, UraLayerShell*>) {
      client.surface = static_cast<UraLayerShell*>(t)->layer_surface->surface;
      client.type = UraSurfaceType::LayerShell;
    } else if constexpr (std::is_same_v<T, UraPopup*>) {
      client.surface = static_cast<UraPopup*>(t)->xdg_popup->base->surface;
      client.type = UraSurfaceType::Popup;
    } else if constexpr (std::is_same_v<T, UraSessionLockSurface*>) {
      client.surface = static_cast<UraSessionLockSurface*>(t)->surface->surface;
      client.type = UraSurfaceType::SessionLock;
    } else {
      std::unreachable();
    }
    return client;
  }

  static UraClient null() {
    auto client = UraClient();
    client.type = UraSurfaceType::Null;
    client.surface = nullptr;
    return client;
  }

  inline bool operator==(const UraClient& rhs) const {
    return this->surface == rhs.surface;
  }

  void focus();
};
} // namespace ura
