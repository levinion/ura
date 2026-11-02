#include "ura/scene/frame_planner.hpp"

#include "ura/core/server.hpp"
#include "ura/scene/scene.hpp"
#include "ura/seat/seat.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/output.hpp"
#include "ura/view/session_lock.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/util/rgb.hpp"
#include "ura/view/view.hpp"

extern "C" {
#include <wlr/util/transform.h>
}

namespace ura {

namespace {

  struct CollectContext {
    FramePlan* plan;
    const wlr_box* output_box;
    int base_x;
    int base_y;
  };

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

  void collect_surface(wlr_surface* surface, int sx, int sy, void* data) {
    auto* context = static_cast<CollectContext*>(data);
    if (!surface->mapped)
      return;

    auto* state = &surface->current;
    if (state->width <= 0 || state->height <= 0)
      return;

    auto* texture = wlr_surface_get_texture(surface);
    if (!texture)
      return;

    FrameSurface item {
    .surface = surface,
    .texture = texture,
    .dst_box = {
      context->base_x + sx,
      context->base_y + sy,
      state->width,
      state->height,
    },
    .transform = state->transform,
    .opacity = 1.f,
  };

    wlr_box intersection;
    if (!wlr_box_intersection(
          &intersection,
          &item.dst_box,
          context->output_box
        ))
      return;

    wlr_surface_get_buffer_source_box(surface, &item.src_box);

    if (auto* alpha = wlr_alpha_modifier_v1_get_surface_state(surface))
      item.opacity = alpha->multiplier;

    if (auto* sync = wlr_linux_drm_syncobj_v1_get_surface_state(surface)) {
      item.wait_timeline = sync->acquire_timeline;
      item.wait_point = sync->acquire_point;
    }

    append_surface(*context->plan, item);
  }

  void collect_toplevel_borders(
    FramePlan& plan,
    UraToplevel* toplevel,
    const SceneItemSnapshot& item
  ) {
    auto color = util::hex2rgba(toplevel->border_color);
    if (!color || (*color)[3] <= 0.f)
      return;

    auto width = static_cast<int>(toplevel->border_width());
    if (width == 0)
      return;

    auto geometry = toplevel->geometry();
    auto add = [&](int x, int y, int w, int h, int index) {
      append_rect(
        plan,
        FrameRect {
          .dst_box = {
            item.transform.x + x,
            item.transform.y + y,
            w,
            h,
          },
          .color = *color,
          .owner = toplevel->id(),
          .index = index,
        }
      );
    };

    add(-width, -width, geometry.width + 2 * width, width, 0);
    add(geometry.width, -width, width, geometry.height + 2 * width, 1);
    add(-width, geometry.height, geometry.width + 2 * width, width, 2);
    add(-width, -width, width, geometry.height + 2 * width, 3);
  }

  void collect_toplevel(FramePlan& plan, const SceneItemSnapshot& item) {
    auto* server = UraServer::get_instance();
    auto* toplevel =
      server->entities->toplevel(UraEntityRegistry::from_id(item.id));
    if (!toplevel || !toplevel->mapped())
      return;

    if (toplevel->is_fullscreen())
      plan.allow_direct_scanout = true;

    auto* xdg_surface = toplevel->xdg_toplevel->base;
    CollectContext context {
      .plan = &plan,
      .output_box = &plan.logical_box,
      .base_x = item.transform.x - xdg_surface->geometry.x,
      .base_y = item.transform.y - xdg_surface->geometry.y,
    };
    // Keep a stable paint order: root surface and subsurfaces, then borders,
    // then xdg popups.
    wlr_surface_for_each_surface(
      xdg_surface->surface,
      collect_surface,
      &context
    );
    collect_toplevel_borders(plan, toplevel, item);
    wlr_xdg_surface_for_each_popup_surface(
      xdg_surface,
      collect_surface,
      &context
    );

    auto* text_input = server->seat->text_input.get();
    for (auto* popup : text_input->popups) {
      if (popup->parent != toplevel)
        continue;
      auto* popup_surface = popup->popup_surface->surface;
      if (!popup_surface->mapped)
        continue;
      CollectContext popup_context {
        .plan = &plan,
        .output_box = &plan.logical_box,
        .base_x = item.transform.x + popup->geometry.x,
        .base_y = item.transform.y + popup->geometry.y,
      };
      wlr_surface_for_each_surface(
        popup_surface,
        collect_surface,
        &popup_context
      );
    }
  }

  void collect_layer_shell(
    FramePlan& plan,
    UraOutput* output,
    const SceneItemSnapshot& item
  ) {
    auto* server = UraServer::get_instance();
    auto* layer_shell =
      server->entities->layer_shell(UraEntityRegistry::from_id(item.id));
    if (!layer_shell || layer_shell->output != output->name)
      return;
    if (!layer_shell->layer_surface->surface->mapped)
      return;

    plan.allow_direct_scanout = true;
    CollectContext context {
      .plan = &plan,
      .output_box = &plan.logical_box,
      .base_x = item.transform.x,
      .base_y = item.transform.y,
    };
    wlr_layer_surface_v1_for_each_surface(
      layer_shell->layer_surface,
      collect_surface,
      &context
    );
  }

  void collect_session_lock(FramePlan& plan, UraOutput* output) {
    auto* lock_surface = output->session_lock_surface;
    if (!lock_surface || !lock_surface->surface->surface->mapped)
      return;

    plan.allow_direct_scanout = true;
    auto geometry = output->logical_geometry();
    CollectContext context {
      .plan = &plan,
      .output_box = &plan.logical_box,
      .base_x = geometry.x,
      .base_y = geometry.y,
    };
    wlr_surface_for_each_surface(
      lock_surface->surface->surface,
      collect_surface,
      &context
    );
  }

  bool has_software_cursor(wlr_output* output) {
    wlr_output_cursor* cursor;
    wl_list_for_each(cursor, &output->cursors, link) {
      if (
        cursor->enabled && cursor->visible && cursor != output->hardware_cursor
      )
        return true;
    }
    return false;
  }

  bool same_box(const wlr_box& lhs, const wlr_box& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width
      && lhs.height == rhs.height;
  }

  void add_damage(FramePlan& plan, const wlr_box& box) {
    wlr_box clipped;
    if (wlr_box_intersection(&clipped, &box, &plan.logical_box))
      plan.damage.push_back(clipped);
  }

} // namespace

void UraFramePlanner::forget_surface(wlr_surface* surface) {
  for (auto& [output, state] : this->outputs) {
    state.entered_surfaces.erase(surface);
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

FramePlan UraFramePlanner::build(UraOutput* output) {
  FramePlan plan;
  plan.output = output->output;
  plan.damage_ring = output->damage_ring();
  plan.logical_box = output->logical_geometry().to_wlr_box();
  plan.background_color = output->background_color;
  if (
    !output->output || !output->output->enabled || plan.logical_box.width <= 0
    || plan.logical_box.height <= 0
  ) {
    this->remove(output);
    return plan;
  }

  auto& state = this->outputs[output];
  auto software_cursor = has_software_cursor(output->output);
  auto full_redraw = !state.initialized
    || !same_box(state.logical_box, plan.logical_box)
    || state.scale != output->output->scale
    || state.transform != output->output->transform || software_cursor
    || state.software_cursor != software_cursor;
  if (full_redraw) {
    plan.full_redraw = true;
    plan.damage.push_back(plan.logical_box);
  }

  for (auto& box : state.orphan_damage) add_damage(plan, box);
  state.orphan_damage.clear();

  auto* server = UraServer::get_instance();
  if (server->seat->locked) {
    collect_session_lock(plan, output);
  } else {
    auto snapshot = server->view->scene->snapshot();
    for (auto& item : snapshot.items) {
      if (!item.visibility.visible || item.visibility.occluded)
        continue;
      if (item.role == SceneRole::Toplevel)
        collect_toplevel(plan, item);
      else if (item.role == SceneRole::LayerShell)
        collect_layer_shell(plan, output, item);
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
        .seq = surface.surface->current.seq,
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
    for (auto& previous : state.surfaces) {
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
      if (previous.seq != surface->surface->current.seq)
        add_damage(plan, surface->dst_box);
    }

    for (auto& surface : current) {
      auto existed = std::ranges::any_of(
        state.surfaces,
        [&](const PreviousSurface& previous) {
          return previous.surface == surface.surface;
        }
      );
      if (!existed)
        add_damage(plan, surface.dst_box);
    }

    for (auto& previous : state.rects) {
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
        std::ranges::any_of(state.rects, [&](const PreviousRect& previous) {
          return previous.owner == rect.owner && previous.index == rect.index;
        });
      if (!existed)
        add_damage(plan, rect.dst_box);
    }
  }

  auto primary_output_for_box = [&](const wlr_box& box) -> UraOutput* {
    UraOutput* best = nullptr;
    int best_area = 0;
    for (auto& [name, candidate] : server->view->outputs) {
      if (!candidate->output || !candidate->output->enabled)
        continue;
      auto geometry = candidate->logical_geometry();
      auto candidate_box = geometry.to_wlr_box();
      wlr_box intersection;
      if (!wlr_box_intersection(&intersection, &box, &candidate_box))
        continue;
      auto area = intersection.width * intersection.height;
      if (!best || area > best_area) {
        best = candidate;
        best_area = area;
      }
    }
    return best;
  };

  std::unordered_set<wlr_surface*> entered_surfaces;
  auto* view = server->view.get();
  for (auto& surface : plan.surfaces) {
    surface.primary_output = primary_output_for_box(surface.dst_box) == output;
    view->track_surface(surface.surface);
    wlr_surface_send_enter(surface.surface, output->output);
    if (surface.primary_output)
      view->notify_scale(surface.surface, output->output->scale);
    entered_surfaces.insert(surface.surface);
  }
  for (auto* surface : state.entered_surfaces) {
    if (!entered_surfaces.contains(surface))
      wlr_surface_send_leave(surface, output->output);
  }

  state.initialized = true;
  state.logical_box = plan.logical_box;
  state.scale = output->output->scale;
  state.transform = output->output->transform;
  state.software_cursor = software_cursor;
  state.entered_surfaces = std::move(entered_surfaces);
  state.surfaces = std::move(current);
  state.rects = std::move(current_rects);
  return plan;
}

} // namespace ura
