#pragma once

#include "ura/ura.hpp"
#include "ura/server.hpp"
#include <list>
#include "ura/workspace.hpp"

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
  std::list<std::unique_ptr<UraWorkSpace>> workspaces;

  static UraOutput* get_instance(wlr_output* output);
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

  void create_workspace();
  void switch_workspace(int index);
  UraWorkSpace* current_workspace;
};

} // namespace ura
