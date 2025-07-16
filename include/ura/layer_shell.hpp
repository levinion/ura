#pragma once

#include "ura/ura.hpp"
#include "ura/output.hpp"

namespace ura {

class UraLayerShell {
public:
  wlr_layer_surface_v1* layer_surface;
  wlr_scene_layer_surface_v1* scene_surface;
  UraOutput* output;
};

} // namespace ura
