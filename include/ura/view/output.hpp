#pragma once

#include "ura/core/server.hpp"
#include "ura/util/vec.hpp"
#include <sol/sol.hpp>
#include "ura/view/workspace.hpp"

namespace ura {

class UraLayerShell;
class UraSessionLockSurface;
class UraPopup;

class UraOutput {
public:
  wlr_output* output;
  std::string name;

  bool dpms_on = true;
  std::optional<wlr_output_mode> mode;

  void init(wlr_output* output);
  static UraOutput* from(wlr_output* output);
  static UraOutput* from(uint64_t id);
  static UraOutput* from(std::string_view name);
  uint64_t id();
  void commit();
  void destroy();
  void set_scale(float scale);
  Vec4<int> physical_geometry();
  Vec4<int> logical_geometry();

  /* Mode */
  void set_dpms_mode(bool flag);

  /* Surfaces */
  Vec<UraLayerShell*> bottom_surfaces;
  Vec<UraLayerShell*> background_surfaces;
  Vec<UraLayerShell*> top_surfaces;
  Vec<UraLayerShell*> overlay_surfaces;
  Vec<UraPopup*> popups;

  /* Layers */
  Vec4<int> usable_area;
  wlr_scene_rect* background;
  UraSessionLockSurface* session_lock_surface = nullptr;

  void update_background();
  bool configure_layers();
  void configure_layer(
    Vec<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area,
    bool exclusive
  );
  Vec<UraLayerShell*>& get_layer_list_by_type(zwlr_layer_shell_v1_layer type);

  /* Workspaces */
  UraWorkSpace* current_workspace = nullptr;
  UraWorkSpace* create_workspace();
  Vec<UraWorkSpace*>& get_workspaces();
  void switch_workspace(UraWorkSpace* workspace);
  void destroy_workspace(UraWorkSpace* workspace);
  UraWorkSpace* get_workspace_at(int index);
  sol::table to_lua_table();
};

} // namespace ura
