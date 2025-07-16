#pragma once

#include "ura/ura.hpp"
#include "ura/server.hpp"
#include "ura/runtime.hpp"

namespace ura {

class UraLayerShell;

class UraOutput {
public:
  wlr_output* output;

  // shell layers
  wlr_scene_tree* bottom;
  wlr_scene_tree* background;
  wlr_scene_tree* top;
  wlr_scene_tree* overlay;

  static UraOutput* get_instance(wlr_output* output) {
    auto outputs = UraServer::get_instance()->runtime->outputs;
    return *std::find_if(outputs.begin(), outputs.end(), [&](auto i) {
      return i->output == output;
    });
  }

  void commit_frame();
  void commit_layers(UraLayerShell* layer_shell);
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
};

} // namespace ura
