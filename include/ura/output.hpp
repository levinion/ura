#pragma once

#include "ura/server.hpp"
#include <list>

namespace ura {

class UraLayerShell;
class UraSessionLockSurface;
class UraPopup;

class UraOutput {
public:
  wlr_output* output;
  bool dpms_on = true;

  void init(wlr_output* output);
  static UraOutput* from(wlr_output* output);
  void fresh_screen();
  wlr_box physical_geometry();
  wlr_box logical_geometry();

  /* Power */
  void set_dpms_mode(bool flag);

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
  wlr_scene_tree* lock_screen;
  wlr_box usable_area;

  UraSessionLockSurface* session_lock_surface = nullptr;

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
  UraWorkSpace* current_workspace = nullptr;
  std::list<std::unique_ptr<UraWorkSpace>> workspaces;
  UraWorkSpace* create_workspace();
  bool switch_workspace(int index);
  bool switch_workspace(UraWorkSpace* workspace);
  void destroy_workspace(int index);
  UraWorkSpace* get_workspace_at(int index);
  sol::table to_lua_table();

private:
  void set_mode(wlr_output_mode* mode);
};

} // namespace ura
