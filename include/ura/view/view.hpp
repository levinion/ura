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

class UraView {
public:
  std::unique_ptr<UraScene> scene;
  std::unique_ptr<UraFramePlanner> frame_planner;
  std::unique_ptr<UraFrameProfiler> frame_profiler;
  std::unique_ptr<UraFrameScheduler> frame_scheduler;
  std::unordered_map<std::string, UraOutput*> outputs;
  std::unordered_map<std::string, UraOutputContext> output_contexts;
  Vec<UraToplevel*> toplevels;
  Vec<UraLayerShell*> layer_shell_surfaces;

  ~UraView();

  static std::unique_ptr<UraView> init(UraEntityRegistry& entities);
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

private:
  struct SurfaceListener;
  std::unordered_map<wlr_surface*, std::unique_ptr<SurfaceListener>>
    surface_listeners;

  static void handle_surface_commit(wl_listener* listener, void* data);
  static void handle_surface_destroy(wl_listener* listener, void* data);
};
} // namespace ura
