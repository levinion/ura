#pragma once

#include "ura/core/server.hpp"
#include "ura/util/vec.hpp"
#include <sol/sol.hpp>

namespace ura {

class UraLayerShell;
class UraSessionLockSurface;

class UraOutputContext {
public:
  Vec<std::string> tags;
  Vec4<int> logical_geometry;
  float scale = 1.0f;
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
  void refresh_geometry();
  Vec4<int> physical_geometry();
  Vec4<int> logical_geometry();
  float scale();

  void set_dpms_mode(bool flag);

  Vec4<int> usable_area;
  UraSessionLockSurface* session_lock_surface = nullptr;

  Vec<UraLayerShell*> layer_shells();
  Vec<UraLayerShell*>& layer_shells_from_layer(zwlr_layer_shell_v1_layer type);
  bool configure_layers();

  void set_tags(Vec<std::string>&& tags);

private:
  wlr_scene_rect* background;
  float last_notified_scale = 1.0f;
  bool last_geometry_enabled = false;
  Vec4<int> last_logical_geometry;
  void update_background();
  void attach_layer_shells();
  void detach_layer_shells();
  void apply_layout(wlr_output_configuration_v1* config);

  Vec<UraLayerShell*> bottom_surfaces;
  Vec<UraLayerShell*> background_surfaces;
  Vec<UraLayerShell*> top_surfaces;
  Vec<UraLayerShell*> overlay_surfaces;

  void configure_layer(
    Vec<UraLayerShell*>& list,
    wlr_box* full_area,
    wlr_box* usable_area,
    bool exclusive
  );

  void save_context();
  std::optional<UraOutputContext> restore_context();
  UraOutputContext context();
};

} // namespace ura
