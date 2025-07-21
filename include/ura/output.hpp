#pragma once

#include "ura/ura.hpp"
#include "ura/server.hpp"
#include <list>
#include "ura/workspace.hpp"
#include "ura/toplevel.hpp"
#include "ura/popup.hpp"

namespace ura {

class UraLayerShell;

class UraOutput {
public:
  wlr_output* output;
  void init(wlr_output* output);
  static UraOutput* from(wlr_output* output);
  void commit_frame();
  void set_mode(wlr_output_mode* mode);

  /* Surfaces */
  std::list<UraLayerShell*> bottom_surfaces;
  std::list<UraLayerShell*> background_surfaces;
  std::list<UraLayerShell*> top_surfaces;
  std::list<UraLayerShell*> overlay_surfaces;
  std::list<UraPopup*> popups;

  /* Layers */
  wlr_scene_tree* background;
  wlr_scene_tree* bottom;
  wlr_scene_tree* normal;
  wlr_scene_tree* floating;
  wlr_scene_tree* top;
  wlr_scene_tree* popup;
  wlr_scene_tree* fullscreen;
  wlr_scene_tree* overlay;
  wlr_box usable_area;

  void configure_layers();
  void configure_layer(
    wlr_scene_tree* layer,
    std::list<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area,
    bool exclusive
  );
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
  std::list<UraLayerShell*>&
  get_layer_list_by_type(zwlr_layer_shell_v1_layer type);

  /* Workspaces */
  UraWorkSpace* current_workspace;
  std::list<std::unique_ptr<UraWorkSpace>> workspaces;
  UraWorkSpace* create_workspace();
  int switch_workspace(int index);
  int switch_workspace(UraWorkSpace* workspace);
  UraWorkSpace* get_workspace_at(int index);
};

} // namespace ura
