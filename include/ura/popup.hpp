#pragma once

#include "ura.hpp"

namespace ura {

class UraPopup {
public:
  wlr_xdg_popup* xdg_popup;
  void init(wlr_xdg_popup* xdg_popup);
  static UraPopup* from(wlr_surface* surface);
  void focus();
};

} // namespace ura
