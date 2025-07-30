#pragma once

#include "ura.hpp"

namespace ura {

class UraPopup {
public:
  wlr_xdg_popup* xdg_popup;
  wlr_scene_tree* scene_tree;
  bool init(wlr_xdg_popup* xdg_popup);
  static UraPopup* from(wlr_surface* surface);
  void commit();
  void destroy();
};

} // namespace ura
