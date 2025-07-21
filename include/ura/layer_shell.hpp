#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraOutput;

class UraLayerShell {
public:
  wlr_layer_surface_v1* layer_surface;
  wlr_scene_layer_surface_v1* scene_surface;
  wlr_scene_tree* scene_tree;
  UraOutput* output;

  void init(wlr_layer_surface_v1* layer_surface);
  static UraLayerShell* from(wlr_layer_surface_v1* layer_surface);
  void focus();
};

} // namespace ura
