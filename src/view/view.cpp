#include "ura/view/view.hpp"
#include <utility>
#include "ura/scene/animation_system.hpp"
#include "ura/scene/layer.hpp"
#include "ura/view/client.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/session_lock.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/cursor.hpp"
#include "ura/seat/seat.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/core/server.hpp"

namespace ura {

struct UraView::SurfaceListener {
  UraView* view;
  wlr_surface* surface;
  wl_listener commit;
  wl_listener destroy;
};

UraView::~UraView() {
  for (auto& [surface, listener] : this->surface_listeners) {
    wl_list_remove(&listener->commit.link);
    wl_list_remove(&listener->destroy.link);
  }
}

std::unique_ptr<UraView> UraView::init(UraEntityRegistry& entities) {
  auto view = std::make_unique<UraView>();
  view->scene = std::make_unique<UraScene>(entities);
  view->animations = std::make_unique<UraAnimationSystem>(*view->scene);
  view->animations->set_destroy_callback([view = view.get()](entt::entity e) {
    view->destroy_scene_entity(e);
  });
  view->frame_planner = std::make_unique<UraFramePlanner>();
  view->frame_profiler = std::make_unique<UraFrameProfiler>();
  view->frame_scheduler = std::make_unique<UraFrameScheduler>(
    *view->frame_planner,
    *view->frame_profiler
  );
  view->scene->set_dirty_callback([view = view.get()] {
    view->schedule_frames();
  });
  view->scene->set_visual_dirty_callback([view = view.get()](
                                           entt::entity,
                                           const Rect<double>& previous,
                                           const Rect<double>& current
                                         ) {
    view->schedule_visual_change(previous, current);
  });
  return view;
}

namespace {

  struct CaptureContext {
    UraView* view;
    RenderTargetResource* resource;
    int window_x;
    int window_y;
  };

  void capture_surface_cb(wlr_surface* surface, int sx, int sy, void* data) {
    auto* context = static_cast<CaptureContext*>(data);
    // unmap() runs after wlr_surface.mapped has been cleared, so do not
    // filter on that flag here. The buffer is still attached during a close.
    if (!surface || !surface->buffer)
      return;

    auto* buffer = surface->buffer->source ? surface->buffer->source
                                           : &surface->buffer->base;
    auto* server = UraServer::get_instance();
    auto* texture = wlr_texture_from_buffer(server->renderer, buffer);
    if (!texture)
      return;

    auto* state = &surface->current;
    FrozenSurface frozen {
      .texture = texture,
      .dst_box = {
        sx - context->window_x,
        sy - context->window_y,
        state->width,
        state->height,
      },
      .transform = state->transform,
      .opacity = 1.f,
      .content_id = context->view->allocate_content_id(),
    };
    wlr_surface_get_buffer_source_box(surface, &frozen.src_box);

    if (auto* alpha = wlr_alpha_modifier_v1_get_surface_state(surface))
      frozen.opacity = alpha->multiplier;

    context->resource->surfaces.push_back(frozen);
  }

} // namespace

uint64_t
UraView::capture_surface(wlr_surface* surface, int window_x, int window_y) {
  if (!surface)
    return 0;

  RenderTargetResource resource;
  CaptureContext context {
    .view = this,
    .resource = &resource,
    .window_x = window_x,
    .window_y = window_y,
  };
  wlr_surface_for_each_surface(surface, capture_surface_cb, &context);
  if (resource.surfaces.empty())
    return 0;

  auto handle = this->next_render_target++;
  this->render_targets[handle] = std::move(resource);
  return handle;
}

uint64_t UraView::allocate_content_id() {
  return this->next_content_id++;
}

const RenderTargetResource* UraView::render_target(uint64_t handle) const {
  auto it = this->render_targets.find(handle);
  return it == this->render_targets.end() ? nullptr : &it->second;
}

void UraView::release_render_target(uint64_t handle) {
  auto it = this->render_targets.find(handle);
  if (it == this->render_targets.end())
    return;
  for (auto& surface : it->second.surfaces) {
    if (surface.texture)
      wlr_texture_destroy(surface.texture);
  }
  this->render_targets.erase(it);
}

void UraView::clear_render_targets() {
  for (auto& [handle, resource] : this->render_targets) {
    for (auto& surface : resource.surfaces) {
      if (surface.texture)
        wlr_texture_destroy(surface.texture);
    }
  }
  this->render_targets.clear();
}

void UraView::destroy_scene_entity(entt::entity entity) {
  if (!this->scene || !this->scene->raw().valid(entity))
    return;
  if (auto* target = this->scene->try_get<RenderTarget>(entity))
    this->release_render_target(target->resource);
  this->scene->destroy(entity);
}

void UraView::handle_surface_commit(wl_listener* listener, void* data) {
  UraView::SurfaceListener* surface_listener = nullptr;
  surface_listener = wl_container_of(listener, surface_listener, commit);
  surface_listener->view->schedule_surface(surface_listener->surface);
}

void UraView::handle_surface_destroy(wl_listener* listener, void* data) {
  UraView::SurfaceListener* surface_listener = nullptr;
  surface_listener = wl_container_of(listener, surface_listener, destroy);
  auto* view = surface_listener->view;
  auto* surface = surface_listener->surface;
  view->frame_planner->forget_surface(surface);
  view->schedule_frames();
  wl_list_remove(&surface_listener->commit.link);
  wl_list_remove(&surface_listener->destroy.link);
  view->surface_listeners.erase(surface);
}

void UraView::track_surface(wlr_surface* surface) {
  if (!surface || this->surface_listeners.contains(surface))
    return;

  auto listener = std::make_unique<SurfaceListener>();
  listener->view = this;
  listener->surface = surface;
  listener->commit.notify = handle_surface_commit;
  listener->destroy.notify = handle_surface_destroy;
  wl_signal_add(&surface->events.commit, &listener->commit);
  wl_signal_add(&surface->events.destroy, &listener->destroy);
  this->surface_listeners[surface] = std::move(listener);
}

void UraView::schedule_surface(wlr_surface* surface) {
  if (!surface)
    return;

  bool scheduled = false;
  wlr_surface_output* surface_output;
  wl_list_for_each(surface_output, &surface->current_outputs, link) {
    auto* output = UraOutput::from(surface_output->output);
    if (!output || !output->output || !output->output->enabled)
      continue;
    this->frame_scheduler->mark_output_dirty(output);
    wlr_output_schedule_frame(output->output);
    scheduled = true;
  }

  if (!scheduled) {
    // A newly created surface has no current_outputs until its first
    // frame is presented and enter is sent. Do not lose that first commit.
    auto* output = this->current_output();
    if (output && output->output && output->output->enabled) {
      this->frame_scheduler->mark_output_dirty(output);
      wlr_output_schedule_frame(output->output);
    } else {
      this->schedule_frames();
    }
  }
}

void UraView::schedule_frames() {
  this->frame_scheduler->invalidate();
  for (auto& [name, output] : this->outputs) {
    if (output->output && output->output->enabled)
      wlr_output_schedule_frame(output->output);
  }
}

void UraView::schedule_visual_change(
  const Rect<double>& previous,
  const Rect<double>& current
) {
  bool scheduled = false;
  for (auto& [name, output] : this->outputs) {
    if (!output->output || !output->output->enabled)
      continue;
    auto geometry = output->logical_geometry();
    if (!previous.intersects(geometry) && !current.intersects(geometry))
      continue;
    this->frame_scheduler->mark_output_dirty(output);
    wlr_output_schedule_frame(output->output);
    scheduled = true;
  }

  // A node can be entirely outside the layout (for example while a close
  // animation leaves the screen). There is nothing to repaint in that case.
  (void)scheduled;
}

void UraView::tick_animations() {
  if (!this->animations)
    return;
  this->animations->tick(AnimationClock::now());
}

UraOutput* UraView::current_output() {
  auto server = UraServer::get_instance();
  auto [px, py] = server->seat->cursor->position();
  auto output = wlr_output_layout_output_at(server->output_layout, px, py);
  if (!output) {
    double x, y;
    wlr_output_layout_closest_point(
      server->output_layout,
      nullptr,
      px,
      py,
      &x,
      &y
    );
    output = wlr_output_layout_output_at(server->output_layout, x, y);
  }
  return output ? UraOutput::from(output) : nullptr;
}

UraOutput* UraView::get_output_by_name(std::string_view name) {
  // TODO: this should be removed when unordered_map supports string_view as a read key
  auto key = std::string(name);
  return this->outputs.contains(key) ? this->outputs[key] : nullptr;
}

std::optional<UraClient> UraView::foreground_client() {
  auto server = UraServer::get_instance();
  auto [x, y] = server->seat->cursor->position();
  return this->client_at(x, y);
}

std::optional<UraClient> UraView::client_at(double lx, double ly) {
  auto server = UraServer::get_instance();

  auto make_client =
    [](wlr_surface* surface, double sx, double sy) -> std::optional<UraClient> {
    if (!surface)
      return {};
    auto client = UraClient::from(surface);
    client.sx = sx;
    client.sy = sy;
    return client;
  };

  auto& snapshot = this->scene->snapshot();
  for (auto item = snapshot.items.rbegin(); item != snapshot.items.rend();
       ++item) {
    if (!item->visibility.visible || item->visibility.occluded)
      continue;

    auto is_lock = item->role == SceneRole::SessionLock;
    if (server->seat->locked != is_lock)
      continue;

    auto entity = UraEntityRegistry::from_id(item->id);
    double sx, sy;

    auto& bounds = item->node.bounds;
    if (bounds.width <= 0 || bounds.height <= 0)
      continue;
    auto& content = item->content;
    auto scale_x = content.geometry.width > 0
      ? bounds.width / static_cast<double>(content.geometry.width)
      : 1.0;
    auto scale_y = content.geometry.height > 0
      ? bounds.height / static_cast<double>(content.geometry.height)
      : 1.0;

    if (item->role == SceneRole::Toplevel) {
      auto* toplevel = server->entities->toplevel(entity);
      if (!toplevel || !toplevel->mapped())
        continue;

      auto local_x = (lx - bounds.x) / scale_x;
      auto local_y = (ly - bounds.y) / scale_y;

      // Input method popups sit above the regular xdg surface tree.
      auto* text_input = server->seat->text_input.get();
      for (auto popup = text_input->popups.rbegin();
           popup != text_input->popups.rend();
           ++popup) {
        if ((*popup)->parent != toplevel)
          continue;
        auto* popup_surface = (*popup)->popup_surface->surface;
        if (!popup_surface->mapped)
          continue;
        auto& geometry = (*popup)->geometry;
        auto* surface = wlr_surface_surface_at(
          popup_surface,
          local_x - geometry.x,
          local_y - geometry.y,
          &sx,
          &sy
        );
        if (surface)
          return make_client(surface, sx, sy);
      }

      auto* xdg_surface = toplevel->xdg_toplevel->base;
      auto* surface = wlr_xdg_surface_surface_at(
        xdg_surface,
        local_x + content.geometry.x,
        local_y + content.geometry.y,
        &sx,
        &sy
      );
      if (surface)
        return make_client(surface, sx, sy);
    } else if (item->role == SceneRole::LayerShell) {
      auto* layer_shell = server->entities->layer_shell(entity);
      if (!layer_shell || !layer_shell->layer_surface->surface->mapped)
        continue;

      auto* surface = wlr_layer_surface_v1_surface_at(
        layer_shell->layer_surface,
        (lx - bounds.x) / scale_x,
        (ly - bounds.y) / scale_y,
        &sx,
        &sy
      );
      if (surface)
        return make_client(surface, sx, sy);
    } else if (item->role == SceneRole::SessionLock) {
      auto* session_lock = server->entities->session_lock(entity);
      if (!session_lock || !session_lock->surface->surface->mapped)
        continue;

      auto* surface = wlr_surface_surface_at(
        session_lock->surface->surface,
        (lx - bounds.x) / scale_x,
        (ly - bounds.y) / scale_y,
        &sx,
        &sy
      );
      if (surface)
        return make_client(surface, sx, sy);
    }
  }

  return {};
}

int UraView::get_z_index_by_type(zwlr_layer_shell_v1_layer type) {
  switch (type) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      return UraSceneLayer::Background;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      return UraSceneLayer::Bottom;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      return UraSceneLayer::Overlay;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      return UraSceneLayer::Top;
    default:
      std::unreachable();
  }
}

void UraView::notify_scale(wlr_surface* surface, double scale) {
  wlr_fractional_scale_v1_notify_scale(surface, scale);
  wlr_surface_set_preferred_buffer_scale(surface, ceil(scale));
}

Vec<UraLayerShell*> UraView::layer_shells() {
  return this->layer_shell_surfaces;
}

} // namespace ura
