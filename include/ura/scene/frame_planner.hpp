#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ura/scene/frame_plan.hpp"

namespace ura {

class UraOutput;

class UraFramePlanner {
public:
  struct OutputState;

  OutputState* prepare_output(UraOutput* output);
  FramePlan build(
    const OutputFrameState& output,
    const OutputState& previous,
    OutputState& next,
    const FrameWorldSnapshot& world
  );
  void finish_plan(UraOutput* output, FramePlan& plan);
  void commit_state(UraOutput* output, OutputState&& next);
  // Discards the candidate frame history while preserving protocol state
  // (entered surfaces and notified scales) after a transient render failure.
  void abort_frame(UraOutput* output);
  // Full output teardown: sends leave and forgets all history.
  void reset_output(UraOutput* output);
  void remove(UraOutput* output);
  void forget_surface(wlr_surface* surface);

  struct PreviousSurface {
    wlr_surface* surface = nullptr;
    uint64_t content_id = 0;
    wlr_box dst_box {};
    enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
    float opacity = 1.f;
    uint32_t seq = 0;
    std::size_t order = 0;
    EffectInstance effect;
    float rotation = 0.f;
    double rotation_center_x = 0.0;
    double rotation_center_y = 0.0;
  };

  struct PreviousRect {
    wlr_box dst_box {};
    std::array<float, 4> color { { 0.f, 0.f, 0.f, 1.f } };
    uint64_t owner = 0;
    int index = 0;
    std::size_t order = 0;
    EffectInstance effect;
    float rotation = 0.f;
    double rotation_center_x = 0.0;
    double rotation_center_y = 0.0;
  };

  struct OutputState {
    bool initialized = false;
    wlr_box logical_box {};
    float scale = 1.f;
    enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
    bool software_cursor = false;
    bool force_full_redraw = false;
    std::unordered_set<wlr_surface*> entered_surfaces;
    std::unordered_map<wlr_surface*, float> notified_scales;
    std::vector<PreviousSurface> surfaces;
    std::vector<PreviousRect> rects;
    std::vector<wlr_box> orphan_damage;
  };

  std::unordered_map<UraOutput*, OutputState> outputs;
};

} // namespace ura
