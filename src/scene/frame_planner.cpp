#include "ura/scene/frame_planner.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace ura {

namespace {

  uint64_t surface_identity(wlr_surface* surface, uint64_t content_id) {
    return content_id != 0
      ? content_id
      : static_cast<uint64_t>(reinterpret_cast<uintptr_t>(surface));
  }

  bool same_box(const wlr_box& lhs, const wlr_box& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width
      && lhs.height == rhs.height;
  }

  wlr_box rotated_box(
    const wlr_box& box,
    float rotation,
    double center_x,
    double center_y
  ) {
    if (rotation == 0.f)
      return box;

    auto c = std::cos(rotation);
    auto s = std::sin(rotation);
    auto rotate = [&](double x, double y) {
      auto dx = x - center_x;
      auto dy = y - center_y;
      auto rx = center_x + dx * c - dy * s;
      auto ry = center_y + dx * s + dy * c;
      return std::array<double, 2> { rx, ry };
    };
    auto p0 = rotate(box.x, box.y);
    auto p1 = rotate(box.x + box.width, box.y);
    auto p2 = rotate(box.x + box.width, box.y + box.height);
    auto p3 = rotate(box.x, box.y + box.height);
    auto left = std::min({ p0[0], p1[0], p2[0], p3[0] });
    auto top = std::min({ p0[1], p1[1], p2[1], p3[1] });
    auto right = std::max({ p0[0], p1[0], p2[0], p3[0] });
    auto bottom = std::max({ p0[1], p1[1], p2[1], p3[1] });
    int x = static_cast<int>(std::floor(left));
    int y = static_cast<int>(std::floor(top));
    int w = static_cast<int>(std::ceil(right)) - x;
    int h = static_cast<int>(std::ceil(bottom)) - y;
    return { x, y, w, h };
  }

  void add_damage(FramePlan& plan, const wlr_box& box) {
    wlr_box clipped;
    if (wlr_box_intersection(&clipped, &box, &plan.logical_box))
      plan.damage.push_back(clipped);
  }

  wlr_box surface_damage_box(const FrameSurface& surface) {
    auto box = rotated_box(
      surface.dst_box,
      surface.rotation,
      surface.rotation_center_x,
      surface.rotation_center_y
    );
    return surface.effect.outset.apply(box);
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
  // Never flash the desktop background color while a lock surface is waiting
  // for its first commit.
  plan.background_color = world.locked
    ? std::array<float, 4> { { 0.f, 0.f, 0.f, 1.f } }
    : output.background_color;
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

      auto cull_box = rotated_box(
        surface.dst_box,
        surface.rotation,
        surface.rotation_center_x,
        surface.rotation_center_y
      );
      cull_box = surface.effect.outset.apply(cull_box);
      wlr_box intersection;
      if (!wlr_box_intersection(&intersection, &cull_box, &plan.logical_box))
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
          .content_id = surface.content_id,
          .effect = surface.effect,
          .rotation = surface.rotation,
          .rotation_center_x = surface.rotation_center_x,
          .rotation_center_y = surface.rotation_center_y,
          .owner = surface.owner,
        }
      );
    } else {
      auto& rect = world.rects[operation.index];
      if (!rect.output_name.empty() && rect.output_name != output.name)
        continue;

      auto rect_box = rotated_box(
        rect.rect.dst_box,
        rect.rect.rotation,
        rect.rect.rotation_center_x,
        rect.rect.rotation_center_y
      );
      wlr_box intersection;
      if (!wlr_box_intersection(&intersection, &rect_box, &plan.logical_box))
        continue;

      append_rect(plan, rect.rect);
    }
  }

  std::vector<PreviousSurface> current;
  current.reserve(plan.surfaces.size());
  std::unordered_map<uint64_t, std::pair<const FrameSurface*, std::size_t>>
    current_by_surface;
  for (std::size_t i = 0; i < plan.surfaces.size(); ++i) {
    auto& surface = plan.surfaces[i];
    current.push_back(
      PreviousSurface {
        .surface = surface.surface,
        .content_id = surface.content_id,
        .dst_box = surface.dst_box,
        .transform = surface.transform,
        .opacity = surface.opacity,
        .seq = surface.seq,
        .order = i,
        .effect = surface.effect,
        .rotation = surface.rotation,
        .rotation_center_x = surface.rotation_center_x,
        .rotation_center_y = surface.rotation_center_y,
      }
    );
    current_by_surface
      [surface_identity(surface.surface, surface.content_id)] = { &surface, i };
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
        .effect = rect.effect,
        .rotation = rect.rotation,
        .rotation_center_x = rect.rotation_center_x,
        .rotation_center_y = rect.rotation_center_y,
      }
    );
  }

  if (!full_redraw) {
    for (auto& previous : previous.surfaces) {
      auto current_it = current_by_surface.find(
        surface_identity(previous.surface, previous.content_id)
      );
      if (current_it == current_by_surface.end()) {
        add_damage(
          plan,
          previous.effect.outset.apply(rotated_box(
            previous.dst_box,
            previous.rotation,
            previous.rotation_center_x,
            previous.rotation_center_y
          ))
        );
        continue;
      }

      auto* surface = current_it->second.first;
      auto order = current_it->second.second;
      if (
        !same_box(previous.dst_box, surface->dst_box)
        || previous.transform != surface->transform
        || previous.opacity != surface->opacity
        || previous.effect != surface->effect || previous.order != order
        || previous.rotation != surface->rotation
        || previous.rotation_center_x != surface->rotation_center_x
        || previous.rotation_center_y != surface->rotation_center_y
      ) {
        add_damage(
          plan,
          previous.effect.outset.apply(rotated_box(
            previous.dst_box,
            previous.rotation,
            previous.rotation_center_x,
            previous.rotation_center_y
          ))
        );
        add_damage(plan, surface_damage_box(*surface));
      }
      if (previous.seq != surface->seq)
        add_damage(plan, surface_damage_box(*surface));
    }

    for (auto& surface : current) {
      auto existed = std::ranges::any_of(
        previous.surfaces,
        [&](const PreviousSurface& previous) {
          return surface_identity(previous.surface, previous.content_id)
            == surface_identity(surface.surface, surface.content_id);
        }
      );
      if (!existed)
        add_damage(
          plan,
          surface.effect.outset.apply(rotated_box(
            surface.dst_box,
            surface.rotation,
            surface.rotation_center_x,
            surface.rotation_center_y
          ))
        );
    }

    for (auto& previous : previous.rects) {
      auto current_it =
        std::ranges::find_if(current_rects, [&](const PreviousRect& rect) {
          return rect.owner == previous.owner && rect.index == previous.index;
        });
      if (current_it == current_rects.end()) {
        add_damage(
          plan,
          previous.effect.outset.apply(rotated_box(
            previous.dst_box,
            previous.rotation,
            previous.rotation_center_x,
            previous.rotation_center_y
          ))
        );
        continue;
      }
      if (
        !same_box(previous.dst_box, current_it->dst_box)
        || previous.color != current_it->color
        || previous.effect != current_it->effect
        || previous.order != current_it->order
        || previous.rotation != current_it->rotation
        || previous.rotation_center_x != current_it->rotation_center_x
        || previous.rotation_center_y != current_it->rotation_center_y
      ) {
        add_damage(
          plan,
          previous.effect.outset.apply(rotated_box(
            previous.dst_box,
            previous.rotation,
            previous.rotation_center_x,
            previous.rotation_center_y
          ))
        );
        add_damage(
          plan,
          current_it->effect.outset.apply(rotated_box(
            current_it->dst_box,
            current_it->rotation,
            current_it->rotation_center_x,
            current_it->rotation_center_y
          ))
        );
      }
    }

    for (auto& rect : current_rects) {
      auto existed =
        std::ranges::any_of(previous.rects, [&](const PreviousRect& previous) {
          return previous.owner == rect.owner && previous.index == rect.index;
        });
      if (!existed)
        add_damage(
          plan,
          rect.effect.outset.apply(rotated_box(
            rect.dst_box,
            rect.rotation,
            rect.rotation_center_x,
            rect.rotation_center_y
          ))
        );
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
