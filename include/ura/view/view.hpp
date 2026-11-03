#pragma once

#include <memory>
#include <unordered_map>
#include "ura/ura.hpp"
#include "ura/scene/frame_planner.hpp"
#include "ura/scene/frame_profile.hpp"
#include "ura/scene/frame_scheduler.hpp"
#include "ura/scene/scene.hpp"
#include "ura/view/output.hpp"

namespace ura {

class UraAnimationSystem;

// One immutable surface captured from a client surface tree. Positions are in
// window-content coordinates (relative to the xdg window geometry origin).
struct FrozenSurface {
  wlr_texture* texture = nullptr;
  wlr_fbox src_box {};
  wlr_box dst_box {};
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  float opacity = 1.f;
  uint64_t content_id = 0;
};

struct RenderTargetResource {
  std::vector<FrozenSurface> surfaces;
};

class UraView {
public:
  std::unique_ptr<UraScene> scene;
  std::unique_ptr<UraAnimationSystem> animations;
  std::unique_ptr<UraFramePlanner> frame_planner;
  std::unique_ptr<UraFrameProfiler> frame_profiler;
  std::unique_ptr<UraFrameScheduler> frame_scheduler;
  std::unordered_map<std::string, UraOutput*> outputs;
  std::unordered_map<std::string, UraOutputContext> output_contexts;
  Vec<UraToplevel*> toplevels;
  Vec<UraLayerShell*> layer_shell_surfaces;

  ~UraView();

  static std::unique_ptr<UraView> init(UraEntityRegistry& entities);

  // Capture the currently attached client buffer into a renderer-owned
  // texture. Used for close animations and other frozen-content effects.
  uint64_t
  capture_surface(wlr_surface* surface, int window_x = 0, int window_y = 0);
  const RenderTargetResource* render_target(uint64_t handle) const;
  uint64_t allocate_content_id();
  void release_render_target(uint64_t handle);
  void clear_render_targets();
  void destroy_scene_entity(entt::entity entity);
  UraOutput* current_output();
  UraOutput* get_output_by_name(std::string_view name);
  std::optional<UraClient> foreground_client();
  std::optional<UraClient> client_at(double lx, double ly);
  int get_z_index_by_type(zwlr_layer_shell_v1_layer type);
  Vec<UraLayerShell*> layer_shells();
  void notify_scale(wlr_surface* surface, double scale);
  void track_surface(wlr_surface* surface);
  void schedule_surface(wlr_surface* surface);
  void schedule_frames();
  void schedule_visual_change(const Rect<double>& previous, const Rect<double>& current);
  void tick_animations();

private:
  struct SurfaceListener;
  std::unordered_map<wlr_surface*, std::unique_ptr<SurfaceListener>>
    surface_listeners;
  std::unordered_map<uint64_t, RenderTargetResource> render_targets;
  uint64_t next_render_target = 1;
  uint64_t next_content_id = 1;

  static void handle_surface_commit(wl_listener* listener, void* data);
  static void handle_surface_destroy(wl_listener* listener, void* data);
};
} // namespace ura
