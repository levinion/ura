#include "ura/scene/frame_world.hpp"

#include <array>
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
    int width = 0;
    int height = 0;
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

  using WorldTask = std::variant<ToplevelTask, LayerTask, LockTask>;

  struct CollectContext {
    WorldChunk* chunk;
    int base_x;
    int base_y;
  };

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

    FrameSurfaceSnapshot item {
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
    .seq = state->seq,
    .output_name = {},
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
    auto add = [&](int x, int y, int w, int h, int index) {
      append_rect(
      chunk,
      FrameRectSnapshot {
        .rect = {
          .dst_box = {
            task.item.transform.x + x,
            task.item.transform.y + y,
            w,
            h,
          },
          .color = task.border_color,
          .owner = task.owner,
          .index = index,
        },
        .output_name = {},
      }
    );
    };

    add(-width, -width, task.width + 2 * width, width, 0);
    add(task.width, -width, width, task.height + 2 * width, 1);
    add(-width, task.height, task.width + 2 * width, width, 2);
    add(-width, -width, width, task.height + 2 * width, 3);
  }

  void build_toplevel(WorldChunk& chunk, const ToplevelTask& task) {
    auto surface_start = chunk.surfaces.size();
    CollectContext context {
      .chunk = &chunk,
      .base_x = task.item.transform.x - task.xdg_surface->geometry.x,
      .base_y = task.item.transform.y - task.xdg_surface->geometry.y,
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
        .base_x = task.item.transform.x + popup.x,
        .base_y = task.item.transform.y + popup.y,
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
    CollectContext context {
      .chunk = &chunk,
      .base_x = task.item.transform.x,
      .base_y = task.item.transform.y,
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
      .base_x = task.base.x,
      .base_y = task.base.y,
    };
    wlr_surface_for_each_surface(task.surface, collect_surface, &context);
    for (std::size_t i = previous_size; i < chunk.surfaces.size(); ++i) {
      chunk.surfaces[i].output_name = task.output_name;
      chunk.surfaces[i].lock_surface = true;
      chunk.surfaces[i].allow_direct_scanout = true;
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
        else
          build_lock(chunk, value);
      },
      task
    );
    return chunk;
  }

  void
  collect_tasks(std::vector<WorldTask>& tasks, const SceneItemSnapshot& item) {
    auto* server = UraServer::get_instance();
    if (item.role == SceneRole::Toplevel) {
      auto* toplevel =
        server->entities->toplevel(UraEntityRegistry::from_id(item.id));
      if (!toplevel || !toplevel->mapped())
        return;

      ToplevelTask task {
        .item = item,
        .xdg_surface = toplevel->xdg_toplevel->base,
        .fullscreen = toplevel->is_fullscreen(),
        .width = toplevel->geometry().width,
        .height = toplevel->geometry().height,
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
    }
  }

} // namespace

FrameWorldSnapshot build_frame_world(UraWorkerPool& pool) {
  FrameWorldSnapshot world;
  auto* server = UraServer::get_instance();
  world.locked = server->seat->locked;

  std::vector<WorldTask> tasks;
  if (world.locked) {
    for (auto& [name, output] : server->view->outputs) {
      auto* lock_surface = output->session_lock_surface;
      if (!lock_surface || !lock_surface->surface->surface->mapped)
        continue;
      auto geometry = output->logical_geometry();
      tasks.emplace_back(
        LockTask {
          .surface = lock_surface->surface->surface,
          .base = geometry.to_wlr_box(),
          .output_name = output->name,
        }
      );
    }
  } else {
    auto& snapshot = server->view->scene->snapshot();
    for (auto& item : snapshot.items) {
      if (!item.visibility.visible || item.visibility.occluded)
        continue;
      collect_tasks(tasks, item);
    }
  }

  world.tasks = tasks.size();
  if (tasks.empty())
    return world;

  std::vector<WorldChunk> chunks(tasks.size());
  pool.parallel_for(tasks.size(), [&](std::size_t index) {
    chunks[index] = build_chunk(tasks[index]);
  });

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
