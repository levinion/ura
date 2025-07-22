#pragma once

#include "ura.hpp"

namespace ura {

class UraPopup {
public:
  wlr_xdg_popup* xdg_popup;
  void init(wlr_xdg_popup* xdg_popup);
};

} // namespace ura
