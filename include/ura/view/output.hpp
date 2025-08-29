#pragma once

#include "ura/core/server.hpp"
#include "ura/util/vec.hpp"

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
  void commit();
  Vec4<int> physical_geometry();
  Vec4<int> logical_geometry();
  int index();

  /* Power */
  void set_dpms_mode(bool flag);
  void set_mode(wlr_output_mode* mode);

  /* Surfaces */
  Vec<UraLayerShell*> bottom_surfaces;
  Vec<UraLayerShell*> background_surfaces;
  Vec<UraLayerShell*> top_surfaces;
  Vec<UraLayerShell*> overlay_surfaces;
  Vec<UraPopup*> popups;

  /* Layers */
  Vec4<int> usable_area;

  UraSessionLockSurface* session_lock_surface = nullptr;

  bool configure_layers();
  void configure_layer(
    Vec<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area,
    bool exclusive
  );
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
  Vec<UraLayerShell*>& get_layer_list_by_type(zwlr_layer_shell_v1_layer type);

  /* Workspaces */
  UraWorkSpace* current_workspace = nullptr;
  Vec<std::unique_ptr<UraWorkSpace>> workspaces;
  UraWorkSpace* create_workspace();
  void switch_workspace(int index);
  void switch_workspace(UraWorkSpace* workspace);
  void destroy_workspace(int index);
  UraWorkSpace* get_workspace_at(int index);
  sol::table to_lua_table();
};

} // namespace ura
