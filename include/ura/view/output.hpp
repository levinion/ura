#pragma once

#include "ura/core/server.hpp"
#include "ura/util/vec.hpp"
#include <sol/sol.hpp>

namespace ura {

class UraLayerShell;
class UraSessionLockSurface;
class UraPopup;

class UraOutputContext {
public:
  Vec<std::string> tags;
};

class UraOutput {
public:
  wlr_output* output;
  std::string name;
  Vec<std::string> tags;

  void init(wlr_output* output);
  static UraOutput* from(wlr_output* output);
  static UraOutput* from(uint64_t id);
  static UraOutput* from(std::string_view name);
  uint64_t id();
  void commit();
  void apply(wlr_output_configuration_v1* config);
  void destroy();
  void set_scale(float scale);
  Vec4<int> physical_geometry();
  Vec4<int> logical_geometry();
  float scale();

  void set_dpms_mode(bool flag);

  Vec<UraLayerShell*> bottom_surfaces;
  Vec<UraLayerShell*> background_surfaces;
  Vec<UraLayerShell*> top_surfaces;
  Vec<UraLayerShell*> overlay_surfaces;
  Vec<UraPopup*> popups;

  Vec4<int> usable_area;
  UraSessionLockSurface* session_lock_surface = nullptr;

  Vec<UraLayerShell*>& get_layer_list_by_type(zwlr_layer_shell_v1_layer type);
  bool configure_layers();

  void set_tags(Vec<std::string>&& tags);
  void focus_lru();

private:
  wlr_scene_rect* background;
  void update_background();

  void configure_layer(
    Vec<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area,
    bool exclusive
  );

  void save_context();
  void restore_context();
  UraOutputContext context();
};

} // namespace ura
