#pragma once

#include "ura/ura.hpp"
#include <string>

namespace ura {

class UraOutput;

class UraLayerShell {
public:
  wlr_layer_surface_v1* layer_surface;
  wlr_scene_layer_surface_v1* scene_surface;
  wlr_scene_tree* scene_tree;
  std::string output;
  zwlr_layer_shell_v1_layer layer;

  void init(wlr_layer_surface_v1* layer_surface);
  static UraLayerShell* from(wlr_surface* surface);
  void focus();
  void unfocus();
  void map();
  void unmap();
  void commit();
  void destroy();

private:
  void dismiss_popups();
};

} // namespace ura
