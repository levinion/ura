#include "ura/scene/frame_planner.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace ura {

namespace {

  bool same_box(const wlr_box& lhs, const wlr_box& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width
      && lhs.height == rhs.height;
  }

  void add_damage(FramePlan& plan, const wlr_box& box) {
    wlr_box clipped;
    if (wlr_box_intersection(&clipped, &box, &plan.logical_box))
      plan.damage.push_back(clipped);
  }

  void append_surface(FramePlan& plan, FrameSurface surface) {
    plan.operations.push_back(
      FrameOperation {
        .kind = FrameOperationKind::Surface,
        .index = plan.surfaces.size(),
      }
    );
    plan.surfaces.push_back(std::move(surface));
  }

  void append_rect(FramePlan& plan, FrameRect rect) {
    plan.operations.push_back(
      FrameOperation {
        .kind = FrameOperationKind::Rect,
        .index = plan.rects.size(),
      }
    );
    plan.rects.push_back(std::move(rect));
  }

} // namespace

UraFramePlanner::OutputState*
UraFramePlanner::prepare_output(UraOutput* output) {
  return &this->outputs.try_emplace(output).first->second;
}

FramePlan UraFramePlanner::build(
  const OutputFrameState& output,
  const OutputState& previous,
  OutputState& next,
  const FrameWorldSnapshot& world
) {
  FramePlan plan;
  plan.output = output.native;
  plan.damage_ring = output.damage_ring;
  plan.logical_box = output.logical_box;
  plan.background_color = output.background_color;
  if (
    !output.enabled || plan.logical_box.width <= 0
    || plan.logical_box.height <= 0
  )
    return plan;

  auto full_redraw = !previous.initialized
    || !same_box(previous.logical_box, plan.logical_box)
    || previous.scale != output.scale || previous.transform != output.transform
    || output.software_cursor
    || previous.software_cursor != output.software_cursor
    || previous.force_full_redraw;
  if (full_redraw) {
    plan.full_redraw = true;
    plan.damage.push_back(plan.logical_box);
  }

  for (auto& box : previous.orphan_damage) add_damage(plan, box);

  for (auto& operation : world.operations) {
    if (operation.kind == FrameOperationKind::Surface) {
      auto& surface = world.surfaces[operation.index];
      if (world.locked && !surface.lock_surface)
        continue;
      if (!surface.output_name.empty() && surface.output_name != output.name)
        continue;

      wlr_box intersection;
      if (!wlr_box_intersection(
            &intersection,
            &surface.dst_box,
            &plan.logical_box
          ))
        continue;

      plan.allow_direct_scanout |= surface.allow_direct_scanout;
      append_surface(
        plan,
        FrameSurface {
          .surface = surface.surface,
          .texture = surface.texture,
          .src_box = surface.src_box,
          .dst_box = surface.dst_box,
          .transform = surface.transform,
          .opacity = surface.opacity,
          .wait_timeline = surface.wait_timeline,
          .wait_point = surface.wait_point,
          .seq = surface.seq,
        }
      );
    } else {
      auto& rect = world.rects[operation.index];
      if (!rect.output_name.empty() && rect.output_name != output.name)
        continue;

      wlr_box intersection;
      if (!wlr_box_intersection(
            &intersection,
            &rect.rect.dst_box,
            &plan.logical_box
          ))
        continue;

      append_rect(plan, rect.rect);
    }
  }

  std::vector<PreviousSurface> current;
  current.reserve(plan.surfaces.size());
  std::unordered_map<wlr_surface*, std::pair<const FrameSurface*, std::size_t>>
    current_by_surface;
  for (std::size_t i = 0; i < plan.surfaces.size(); ++i) {
    auto& surface = plan.surfaces[i];
    current.push_back(
      PreviousSurface {
        .surface = surface.surface,
        .dst_box = surface.dst_box,
        .transform = surface.transform,
        .opacity = surface.opacity,
        .seq = surface.seq,
        .order = i,
      }
    );
    current_by_surface[surface.surface] = { &surface, i };
  }

  std::vector<PreviousRect> current_rects;
  current_rects.reserve(plan.rects.size());
  for (std::size_t i = 0; i < plan.rects.size(); ++i) {
    auto& rect = plan.rects[i];
    current_rects.push_back(
      PreviousRect {
        .dst_box = rect.dst_box,
        .color = rect.color,
        .owner = rect.owner,
        .index = rect.index,
        .order = i,
      }
    );
  }

  if (!full_redraw) {
    for (auto& previous : previous.surfaces) {
      auto current_it = current_by_surface.find(previous.surface);
      if (current_it == current_by_surface.end()) {
        add_damage(plan, previous.dst_box);
        continue;
      }

      auto* surface = current_it->second.first;
      auto order = current_it->second.second;
      if (
        !same_box(previous.dst_box, surface->dst_box)
        || previous.transform != surface->transform
        || previous.opacity != surface->opacity || previous.order != order
      ) {
        add_damage(plan, previous.dst_box);
        add_damage(plan, surface->dst_box);
      }
      if (previous.seq != surface->seq)
        add_damage(plan, surface->dst_box);
    }

    for (auto& surface : current) {
      auto existed = std::ranges::any_of(
        previous.surfaces,
        [&](const PreviousSurface& previous) {
          return previous.surface == surface.surface;
        }
      );
      if (!existed)
        add_damage(plan, surface.dst_box);
    }

    for (auto& previous : previous.rects) {
      auto current_it =
        std::ranges::find_if(current_rects, [&](const PreviousRect& rect) {
          return rect.owner == previous.owner && rect.index == previous.index;
        });
      if (current_it == current_rects.end()) {
        add_damage(plan, previous.dst_box);
        continue;
      }
      if (
        !same_box(previous.dst_box, current_it->dst_box)
        || previous.color != current_it->color
        || previous.order != current_it->order
      ) {
        add_damage(plan, previous.dst_box);
        add_damage(plan, current_it->dst_box);
      }
    }

    for (auto& rect : current_rects) {
      auto existed =
        std::ranges::any_of(previous.rects, [&](const PreviousRect& previous) {
          return previous.owner == rect.owner && previous.index == rect.index;
        });
      if (!existed)
        add_damage(plan, rect.dst_box);
    }
  }

  next = OutputState {};
  next.initialized = true;
  next.logical_box = plan.logical_box;
  next.scale = output.scale;
  next.transform = output.transform;
  next.software_cursor = output.software_cursor;
  next.surfaces = std::move(current);
  next.rects = std::move(current_rects);
  return plan;
}

void UraFramePlanner::commit_state(UraOutput* output, OutputState&& next) {
  auto it = this->outputs.find(output);
  if (it == this->outputs.end())
    return;
  auto entered = std::move(it->second.entered_surfaces);
  auto scales = std::move(it->second.notified_scales);
  it->second = std::move(next);
  it->second.entered_surfaces = std::move(entered);
  it->second.notified_scales = std::move(scales);
}

void UraFramePlanner::abort_frame(UraOutput* output) {
  auto it = this->outputs.find(output);
  if (it == this->outputs.end())
    return;

  // Keep the last successfully presented surface history and the protocol
  // association already announced by finish_plan(). Only force the next plan
  // to repaint the whole output so a failed frame cannot leave stale pixels.
  it->second.force_full_redraw = true;
}

} // namespace ura
