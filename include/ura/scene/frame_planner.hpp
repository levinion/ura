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
  FramePlan build(UraOutput* output);
  void remove(UraOutput* output);
  void forget_surface(wlr_surface* surface);

private:
  struct PreviousSurface {
    wlr_surface* surface = nullptr;
    wlr_box dst_box {};
    enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
    float opacity = 1.f;
    uint32_t seq = 0;
    std::size_t order = 0;
  };

  struct PreviousRect {
    wlr_box dst_box {};
    std::array<float, 4> color { { 0.f, 0.f, 0.f, 1.f } };
    uint64_t owner = 0;
    int index = 0;
    std::size_t order = 0;
  };

  struct OutputState {
    bool initialized = false;
    wlr_box logical_box {};
    float scale = 1.f;
    enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
    bool software_cursor = false;
    std::unordered_set<wlr_surface*> entered_surfaces;
    std::vector<PreviousSurface> surfaces;
    std::vector<PreviousRect> rects;
    std::vector<wlr_box> orphan_damage;
  };

  std::unordered_map<UraOutput*, OutputState> outputs;
};

} // namespace ura
