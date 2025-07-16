#pragma once

#include "ura/ura.hpp"
#include "ura/server.hpp"
#include "ura/runtime.hpp"
#include "list"

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

  std::list<UraLayerShell*> bottom_surfaces;
  std::list<UraLayerShell*> background_surfaces;
  std::list<UraLayerShell*> top_surfaces;
  std::list<UraLayerShell*> overlay_surfaces;

  static UraOutput* get_instance(wlr_output* output) {
    auto outputs = UraServer::get_instance()->runtime->outputs;
    return *std::find_if(outputs.begin(), outputs.end(), [&](auto i) {
      return i->output == output;
    });
  }

  void commit_frame();

  void configure_layers();

  void configure_layer(
    wlr_scene_tree* layer,
    std::list<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area
  );
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
  std::list<UraLayerShell*>&
  get_layer_list_by_type(zwlr_layer_shell_v1_layer type);
};

} // namespace ura
