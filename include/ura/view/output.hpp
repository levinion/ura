#pragma once

#include "ura/core/entity.hpp"
#include "ura/core/server.hpp"
#include "ura/util/vec.hpp"
#include "ura/util/rect.hpp"
#include <sol/sol.hpp>
#include <array>

namespace ura {

class UraLayerShell;
class UraSessionLockSurface;

class UraOutputContext {
public:
  Vec<std::string> tags;
  Rect<int> logical_geometry;
  float scale = 1.0f;
};

class UraOutput {
public:
  entt::entity entity { entt::null };
  wlr_output* output;
  std::string name;
  Vec<std::string> tags;
  std::array<float, 4> background_color { { 0.f, 0.f, 0.f, 1.f } };

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
  Rect<int> physical_geometry();
  Rect<int> logical_geometry();
  float scale();

  void set_dpms_mode(bool flag);
  wlr_damage_ring* damage_ring();
  // Slots used by the frame renderer to hold the previous swapchain buffers.
  // Some drivers (notably NVIDIA with DSC) keep reading recently released
  // buffers; two retained slots give the driver a three-buffer rotation.
  std::array<wlr_buffer*, 2>* retained_buffers();

  Rect<int> usable_area;
  UraSessionLockSurface* session_lock_surface = nullptr;

  Vec<UraLayerShell*> layer_shells();
  Vec<UraLayerShell*>& layer_shells_from_layer(zwlr_layer_shell_v1_layer type);
  bool configure_layers();

  void set_tags(Vec<std::string>&& tags);

private:
  wlr_damage_ring frame_damage_ring;
  std::array<wlr_buffer*, 2> retained_frame_buffers { nullptr, nullptr };
  wlr_drm_syncobj_timeline* render_timeline = nullptr;
  uint64_t render_point = 0;
  float last_notified_scale = 1.0f;
  bool last_geometry_enabled = false;
  Rect<int> last_logical_geometry;
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
