#include "ura/scene/frame_planner.hpp"

#include <unordered_set>
#include <vector>

#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"

namespace ura {

void UraFramePlanner::finish_plan(UraOutput* output, FramePlan& plan) {
  auto state_it = this->outputs.find(output);
  if (state_it == this->outputs.end())
    return;
  auto& state = state_it->second;

  struct Candidate {
    UraOutput* output;
    wlr_box box;
  };

  std::vector<Candidate> candidates;
  auto* server = UraServer::get_instance();
  candidates.reserve(server->view->outputs.size());
  for (auto& [name, candidate] : server->view->outputs) {
    if (!candidate->output || !candidate->output->enabled)
      continue;
    candidates.push_back(
      Candidate {
        .output = candidate,
        .box = candidate->logical_geometry().to_wlr_box(),
      }
    );
  }

  auto primary_output_for_box = [&](const wlr_box& box) -> UraOutput* {
    UraOutput* best = nullptr;
    int best_area = 0;
    for (auto& candidate : candidates) {
      wlr_box intersection;
      if (!wlr_box_intersection(&intersection, &box, &candidate.box))
        continue;
      auto area = intersection.width * intersection.height;
      if (!best || area > best_area) {
        best = candidate.output;
        best_area = area;
      }
    }
    return best;
  };

  std::unordered_set<wlr_surface*> entered_surfaces;
  entered_surfaces.reserve(plan.surfaces.size());

  auto* view = server->view.get();
  auto scale = output->output ? output->output->scale : 1.f;
  for (auto& surface : plan.surfaces) {
    surface.primary_output = primary_output_for_box(surface.dst_box) == output;
    view->track_surface(surface.surface);

    bool newly_entered = !state.entered_surfaces.contains(surface.surface);
    if (newly_entered && output->output)
      wlr_surface_send_enter(surface.surface, output->output);

    if (surface.primary_output && output->output) {
      auto scale_it = state.notified_scales.find(surface.surface);
      if (
        scale_it == state.notified_scales.end() || scale_it->second != scale
      ) {
        view->notify_scale(surface.surface, output->output->scale);
        state.notified_scales[surface.surface] = scale;
      }
    } else {
      state.notified_scales.erase(surface.surface);
    }

    entered_surfaces.insert(surface.surface);
  }

  if (output->output) {
    for (auto* surface : state.entered_surfaces) {
      if (entered_surfaces.contains(surface))
        continue;
      wlr_surface_send_leave(surface, output->output);
      state.notified_scales.erase(surface);
    }
  }
  state.entered_surfaces = std::move(entered_surfaces);
}

void UraFramePlanner::reset_output(UraOutput* output) {
  auto it = this->outputs.find(output);
  if (it == this->outputs.end())
    return;
  if (output->output) {
    for (auto* surface : it->second.entered_surfaces)
      wlr_surface_send_leave(surface, output->output);
  }
  it->second = OutputState {};
}

void UraFramePlanner::forget_surface(wlr_surface* surface) {
  for (auto& [output, state] : this->outputs) {
    state.entered_surfaces.erase(surface);
    state.notified_scales.erase(surface);
    for (auto& previous : state.surfaces) {
      if (previous.surface == surface)
        state.orphan_damage.push_back(previous.dst_box);
    }
    std::erase_if(state.surfaces, [&](const PreviousSurface& previous) {
      return previous.surface == surface;
    });
  }
}

void UraFramePlanner::remove(UraOutput* output) {
  auto it = this->outputs.find(output);
  if (it == this->outputs.end())
    return;
  if (output->output) {
    for (auto* surface : it->second.entered_surfaces)
      wlr_surface_send_leave(surface, output->output);
  }
  this->outputs.erase(it);
}

} // namespace ura
