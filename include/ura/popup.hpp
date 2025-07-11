#pragma once

#include "ura.hpp"

namespace ura {

class UraPopup {
public:
  wlr_xdg_popup* xdg_popup;
  wl_listener commit;
  wl_listener destroy;
};

} // namespace ura
