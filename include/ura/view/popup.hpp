#pragma once

#include <memory>
#include "ura/ura.hpp"

namespace ura {

class UraClient;

class UraPopup {
public:
  wlr_xdg_popup* xdg_popup;
  wlr_scene_tree* scene_tree;
  std::unique_ptr<UraClient> parent;
  bool init(wlr_xdg_popup* xdg_popup);
  static UraPopup* from(wlr_surface* surface);
  static UraPopup* from(uint64_t id);
  void commit();
  void destroy();
  uint64_t id();
  void unfocus();

private:
  void unconstrain();
};

} // namespace ura
