#include "ura/scene/frame_world.hpp"

#include <array>
#include <cmath>
#include <string_view>
#include <variant>
#include <vector>

#include "ura/core/server.hpp"
#include "ura/scene/scene.hpp"
#include "ura/seat/seat.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/util/rgb.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/output.hpp"
#include "ura/view/session_lock.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/view.hpp"

namespace ura {

namespace {

  struct WorldChunk {
    std::vector<FrameSurfaceSnapshot> surfaces;
    std::vector<FrameRectSnapshot> rects;
    std::vector<FrameOperationSnapshot> operations;
  };

  struct InputPopupTask {
    wlr_surface* surface = nullptr;
    int x = 0;
    int y = 0;
  };

  struct ToplevelTask {
    SceneItemSnapshot item;
    wlr_xdg_surface* xdg_surface = nullptr;
    bool fullscreen = false;
    bool has_border = false;
    std::array<float, 4> border_color {};
    int border_width = 0;
    uint64_t owner = 0;
    std::vector<InputPopupTask> input_popups;
  };

  struct LayerTask {
    SceneItemSnapshot item;
    wlr_layer_surface_v1* layer_surface = nullptr;
    std::string_view output_name;
  };

  struct LockTask {
    wlr_surface* surface = nullptr;
    wlr_box base {};
    std::string_view output_name;
  };

  struct FrozenTask {
    SceneItemSnapshot item;
  };

  using WorldTask = std::variant<ToplevelTask, LayerTask, LockTask, FrozenTask>;

  struct CollectContext {
    WorldChunk* chunk;
    double bounds_x = 0.0;
    double bounds_y = 0.0;
    double scale_x = 1.0;
    double scale_y = 1.0;
    // Offset in root-surface coordinates before scaling. For a toplevel this
    // is -window_geometry.origin; for a popup it is the popup offset relative
    // to the parent window geometry.
    double offset_x = 0.0;
    double offset_y = 0.0;
    EffectInstance effect;
    float rotation = 0.f;
    double rotation_center_x = 0.0;
    double rotation_center_y = 0.0;
    uint64_t owner = 0;
  };

  int scaled_edge(double origin, double local, double scale) {
    return static_cast<int>(std::lround(origin + local * scale));
  }

  void append_surface(WorldChunk& chunk, FrameSurfaceSnapshot surface) {
    chunk.operations.push_back(
      FrameOperationSnapshot {
        .kind = FrameOperationKind::Surface,
        .index = chunk.surfaces.size(),
      }
    );
    chunk.surfaces.push_back(std::move(surface));
  }

  void append_rect(WorldChunk& chunk, FrameRectSnapshot rect) {
    chunk.operations.push_back(
      FrameOperationSnapshot {
        .kind = FrameOperationKind::Rect,
        .index = chunk.rects.size(),
      }
    );
    chunk.rects.push_back(std::move(rect));
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

    auto local_x = static_cast<double>(sx) + context->offset_x;
    auto local_y = static_cast<double>(sy) + context->offset_y;
    auto left = scaled_edge(context->bounds_x, local_x, context->scale_x);
    auto top = scaled_edge(context->bounds_y, local_y, context->scale_y);
    auto right = scaled_edge(
      context->bounds_x,
      local_x + static_cast<double>(state->width),
      context->scale_x
    );
    auto bottom = scaled_edge(
      context->bounds_y,
      local_y + static_cast<double>(state->height),
      context->scale_y
    );

    FrameSurfaceSnapshot item {
    .surface = surface,
    .texture = texture,
    .dst_box = {
      left,
      top,
      std::max(1, right - left),
      std::max(1, bottom - top),
    },
    .transform = state->transform,
    .opacity = 1.f,
    .seq = state->seq,
    .output_name = {},
    .effect = context->effect,
    .rotation = context->rotation,
    .rotation_center_x = context->rotation_center_x,
    .rotation_center_y = context->rotation_center_y,
    .owner = context->owner,
  };
    wlr_surface_get_buffer_source_box(surface, &item.src_box);

    if (auto* alpha = wlr_alpha_modifier_v1_get_surface_state(surface))
      item.opacity = alpha->multiplier;

    if (auto* sync = wlr_linux_drm_syncobj_v1_get_surface_state(surface)) {
      item.wait_timeline = sync->acquire_timeline;
      item.wait_point = sync->acquire_point;
    }

    append_surface(*context->chunk, item);
  }

  void collect_toplevel_borders(WorldChunk& chunk, const ToplevelTask& task) {
    if (!task.has_border || task.border_width <= 0)
      return;

    auto width = task.border_width;
    auto x = static_cast<int>(std::lround(task.item.node.bounds.x));
    auto y = static_cast<int>(std::lround(task.item.node.bounds.y));
    auto w = static_cast<int>(std::lround(task.item.node.bounds.width));
    auto h = static_cast<int>(std::lround(task.item.node.bounds.height));
    auto add = [&](int bx, int by, int bw, int bh, int index) {
      append_rect(
      chunk,
      FrameRectSnapshot {
        .rect = {
          .dst_box = { x + bx, y + by, bw, bh },
          .color = task.border_color,
          .owner = task.owner,
          .index = index,
          .effect = task.item.effect,
          .rotation = task.item.node.rotation,
          .rotation_center_x =
            task.item.node.bounds.x + task.item.node.bounds.width / 2.0,
          .rotation_center_y =
            task.item.node.bounds.y + task.item.node.bounds.height / 2.0,
        },
        .output_name = {},
      }
    );
    };

    add(-width, -width, w + 2 * width, width, 0);
    add(w, -width, width, h + 2 * width, 1);
    add(-width, h, w + 2 * width, width, 2);
    add(-width, -width, width, h + 2 * width, 3);
  }

  void build_toplevel(WorldChunk& chunk, const ToplevelTask& task) {
    auto surface_start = chunk.surfaces.size();
    auto& bounds = task.item.node.bounds;
    auto& content = task.item.content;
    auto scale_x = content.geometry.width > 0
      ? bounds.width / static_cast<double>(content.geometry.width)
      : 1.0;
    auto scale_y = content.geometry.height > 0
      ? bounds.height / static_cast<double>(content.geometry.height)
      : 1.0;

    CollectContext context {
      .chunk = &chunk,
      .bounds_x = bounds.x,
      .bounds_y = bounds.y,
      .scale_x = scale_x,
      .scale_y = scale_y,
      .offset_x = -static_cast<double>(content.geometry.x),
      .offset_y = -static_cast<double>(content.geometry.y),
      .effect = task.item.effect,
      .rotation = task.item.node.rotation,
      .rotation_center_x = bounds.x + bounds.width / 2.0,
      .rotation_center_y = bounds.y + bounds.height / 2.0,
      .owner = task.owner,
    };
    wlr_surface_for_each_surface(
      task.xdg_surface->surface,
      collect_surface,
      &context
    );
    collect_toplevel_borders(chunk, task);
    wlr_xdg_surface_for_each_popup_surface(
      task.xdg_surface,
      collect_surface,
      &context
    );

    for (auto& popup : task.input_popups) {
      CollectContext popup_context {
        .chunk = &chunk,
        .bounds_x = bounds.x,
        .bounds_y = bounds.y,
        .scale_x = scale_x,
        .scale_y = scale_y,
        .offset_x = static_cast<double>(popup.x),
        .offset_y = static_cast<double>(popup.y),
        .effect = task.item.effect,
        .rotation = task.item.node.rotation,
        .rotation_center_x = bounds.x + bounds.width / 2.0,
        .rotation_center_y = bounds.y + bounds.height / 2.0,
        .owner = task.owner,
      };
      wlr_surface_for_each_surface(
        popup.surface,
        collect_surface,
        &popup_context
      );
    }

    for (std::size_t i = surface_start; i < chunk.surfaces.size(); ++i)
      chunk.surfaces[i].allow_direct_scanout = task.fullscreen;
  }

  void build_layer_shell(WorldChunk& chunk, const LayerTask& task) {
    auto previous_size = chunk.surfaces.size();
    auto& bounds = task.item.node.bounds;
    auto& content = task.item.content;
    auto scale_x = content.geometry.width > 0
      ? bounds.width / static_cast<double>(content.geometry.width)
      : 1.0;
    auto scale_y = content.geometry.height > 0
      ? bounds.height / static_cast<double>(content.geometry.height)
      : 1.0;
    CollectContext context {
      .chunk = &chunk,
      .bounds_x = bounds.x,
      .bounds_y = bounds.y,
      .scale_x = scale_x,
      .scale_y = scale_y,
      .effect = task.item.effect,
      .rotation = task.item.node.rotation,
      .rotation_center_x = bounds.x + bounds.width / 2.0,
      .rotation_center_y = bounds.y + bounds.height / 2.0,
      .owner = task.item.id,
    };
    wlr_layer_surface_v1_for_each_surface(
      task.layer_surface,
      collect_surface,
      &context
    );
    for (std::size_t i = previous_size; i < chunk.surfaces.size(); ++i) {
      chunk.surfaces[i].output_name = task.output_name;
      chunk.surfaces[i].allow_direct_scanout = true;
    }
  }

  void build_lock(WorldChunk& chunk, const LockTask& task) {
    auto previous_size = chunk.surfaces.size();
    CollectContext context {
      .chunk = &chunk,
      .bounds_x = static_cast<double>(task.base.x),
      .bounds_y = static_cast<double>(task.base.y),
      .effect = {},
      .rotation = 0.f,
      .rotation_center_x = 0.0,
      .rotation_center_y = 0.0,
      .owner = 0,
    };
    wlr_surface_for_each_surface(task.surface, collect_surface, &context);
    for (std::size_t i = previous_size; i < chunk.surfaces.size(); ++i) {
      chunk.surfaces[i].output_name = task.output_name;
      chunk.surfaces[i].lock_surface = true;
      chunk.surfaces[i].allow_direct_scanout = true;
    }
  }

  void build_frozen(WorldChunk& chunk, const FrozenTask& task) {
    auto* server = UraServer::get_instance();
    auto* resource = server->view->render_target(task.item.render_target);
    if (!resource)
      return;

    auto& bounds = task.item.node.bounds;
    auto& content = task.item.content;
    auto scale_x = content.geometry.width > 0
      ? bounds.width / static_cast<double>(content.geometry.width)
      : 1.0;
    auto scale_y = content.geometry.height > 0
      ? bounds.height / static_cast<double>(content.geometry.height)
      : 1.0;

    for (auto& frozen : resource->surfaces) {
      if (!frozen.texture)
        continue;

      auto left = scaled_edge(bounds.x, frozen.dst_box.x, scale_x);
      auto top = scaled_edge(bounds.y, frozen.dst_box.y, scale_y);
      auto right = scaled_edge(
        bounds.x,
        static_cast<double>(frozen.dst_box.x + frozen.dst_box.width),
        scale_x
      );
      auto bottom = scaled_edge(
        bounds.y,
        static_cast<double>(frozen.dst_box.y + frozen.dst_box.height),
        scale_y
      );

      FrameSurfaceSnapshot surface {
        .surface = nullptr,
        .texture = frozen.texture,
        .src_box = frozen.src_box,
        .dst_box = {
          left,
          top,
          std::max(1, right - left),
          std::max(1, bottom - top),
        },
        .transform = frozen.transform,
        .opacity = frozen.opacity * task.item.opacity.value,
        .seq = 0,
        .output_name = {},
        .content_id = frozen.content_id,
        .effect = task.item.effect,
        .rotation = task.item.node.rotation,
        .rotation_center_x = bounds.x + bounds.width / 2.0,
        .rotation_center_y = bounds.y + bounds.height / 2.0,
        .owner = task.item.id,
      };
      append_surface(chunk, surface);
    }
  }

  WorldChunk build_chunk(const WorldTask& task) {
    WorldChunk chunk;
    std::visit(
      [&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, ToplevelTask>)
          build_toplevel(chunk, value);
        else if constexpr (std::is_same_v<T, LayerTask>)
          build_layer_shell(chunk, value);
        else if constexpr (std::is_same_v<T, LockTask>)
          build_lock(chunk, value);
        else
          build_frozen(chunk, value);
      },
      task
    );
    return chunk;
  }

  void
  collect_tasks(std::vector<WorldTask>& tasks, const SceneItemSnapshot& item) {
    auto* server = UraServer::get_instance();
    if (item.render_target != 0) {
      tasks.emplace_back(FrozenTask { .item = item });
      return;
    }

    if (item.role == SceneRole::Toplevel) {
      auto* toplevel =
        server->entities->toplevel(UraEntityRegistry::from_id(item.id));
      if (!toplevel || !toplevel->mapped())
        return;

      ToplevelTask task {
        .item = item,
        .xdg_surface = toplevel->xdg_toplevel->base,
        .fullscreen = toplevel->is_fullscreen(),
        .owner = toplevel->id(),
        .input_popups = {},
      };
      if (auto color = util::hex2rgba(toplevel->border_color); color) {
        task.has_border = (*color)[3] > 0.f;
        task.border_color = *color;
        task.border_width = static_cast<int>(toplevel->border_width());
      }

      for (auto* popup : server->seat->text_input->popups) {
        if (popup->parent != toplevel)
          continue;
        auto* surface = popup->popup_surface->surface;
        if (!surface->mapped)
          continue;
        task.input_popups.push_back(
          InputPopupTask {
            .surface = surface,
            .x = popup->geometry.x,
            .y = popup->geometry.y,
          }
        );
      }
      tasks.emplace_back(std::move(task));
    } else if (item.role == SceneRole::LayerShell) {
      auto* layer_shell =
        server->entities->layer_shell(UraEntityRegistry::from_id(item.id));
      if (!layer_shell || !layer_shell->layer_surface->surface->mapped)
        return;
      tasks.emplace_back(
        LayerTask {
          .item = item,
          .layer_surface = layer_shell->layer_surface,
          .output_name = layer_shell->output,
        }
      );
    } else if (item.role == SceneRole::SessionLock) {
      auto* session_lock =
        server->entities->session_lock(UraEntityRegistry::from_id(item.id));
      if (
        !session_lock || !session_lock->output
        || !session_lock->surface->surface->mapped
      )
        return;

      tasks.emplace_back(
        LockTask {
          .surface = session_lock->surface->surface,
          .base = {
            .x = static_cast<int>(std::lround(item.node.bounds.x)),
            .y = static_cast<int>(std::lround(item.node.bounds.y)),
            .width = static_cast<int>(std::lround(item.node.bounds.width)),
            .height = static_cast<int>(std::lround(item.node.bounds.height)),
          },
          .output_name = session_lock->output ? session_lock->output->name
                                              : std::string_view {},
        }
      );
    }
  }

} // namespace

FrameWorldSnapshot build_frame_world() {
  FrameWorldSnapshot world;
  auto* server = UraServer::get_instance();
  world.locked = server->seat->locked;

  std::vector<WorldTask> tasks;
  auto& snapshot = server->view->scene->snapshot();
  for (auto& item : snapshot.items) {
    auto is_lock = item.role == SceneRole::SessionLock;
    // A locked seat renders lock surfaces only. Keeping this gate in the
    // planner/world boundary prevents any normal scene node from leaking
    // through during or after a lock transition.
    if (world.locked != is_lock)
      continue;
    if (!item.visibility.visible || item.visibility.occluded)
      continue;
    collect_tasks(tasks, item);
  }

  world.tasks = tasks.size();
  if (tasks.empty())
    return world;

  // Surface traversal, texture lookup and timeline state access stay on the
  // main thread. Only the resulting immutable snapshot may cross into the
  // worker pool for pure frame planning.
  std::vector<WorldChunk> chunks;
  chunks.reserve(tasks.size());
  for (auto& task : tasks) chunks.push_back(build_chunk(task));

  for (auto& chunk : chunks) {
    auto surface_offset = world.surfaces.size();
    auto rect_offset = world.rects.size();
    for (auto& surface : chunk.surfaces)
      world.surfaces.push_back(std::move(surface));
    for (auto& rect : chunk.rects) world.rects.push_back(std::move(rect));
    for (auto& operation : chunk.operations) {
      if (operation.kind == FrameOperationKind::Surface)
        operation.index += surface_offset;
      else
        operation.index += rect_offset;
      world.operations.push_back(operation);
    }
  }

  return world;
}

} // namespace ura
