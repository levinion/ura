#include "ura/view/view.hpp"
#include <utility>
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
  view->frame_planner = std::make_unique<UraFramePlanner>();
  view->scene->set_dirty_callback([view = view.get()] {
    view->schedule_frames();
  });
  return view;
}

void UraView::handle_surface_commit(wl_listener* listener, void* data) {
  UraView::SurfaceListener* surface_listener = nullptr;
  surface_listener = wl_container_of(listener, surface_listener, commit);
  surface_listener->view->schedule_frames();
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

void UraView::schedule_frames() {
  for (auto& [name, output] : this->outputs) {
    if (output->output && output->output->enabled)
      wlr_output_schedule_frame(output->output);
  }
}

UraOutput* UraView::current_output() {
  auto server = UraServer::get_instance();
  auto pos = server->seat->cursor->position();
  auto output =
    wlr_output_layout_output_at(server->output_layout, pos.x, pos.y);
  if (!output) {
    double x, y;
    wlr_output_layout_closest_point(
      server->output_layout,
      nullptr,
      pos.x,
      pos.y,
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
  auto pos = server->seat->cursor->position();
  return this->client_at(pos.x, pos.y);
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

  // A session lock owns the whole output. It is not part of UraScene yet, but
  // its geometry is known from the output layout and must win over every
  // regular window while the seat is locked.
  if (server->seat->locked) {
    auto* output = wlr_output_layout_output_at(server->output_layout, lx, ly);
    if (!output)
      return {};
    auto* ura_output = UraOutput::from(output);
    if (!ura_output || !ura_output->session_lock_surface)
      return {};
    if (!ura_output->session_lock_surface->surface->surface->mapped)
      return {};
    auto geometry = ura_output->logical_geometry();
    double sx, sy;
    auto* surface = wlr_surface_surface_at(
      ura_output->session_lock_surface->surface->surface,
      lx - geometry.x,
      ly - geometry.y,
      &sx,
      &sy
    );
    return make_client(surface, sx, sy);
  }

  auto snapshot = this->scene->snapshot();
  for (auto item = snapshot.items.rbegin(); item != snapshot.items.rend();
       ++item) {
    if (!item->visibility.visible || item->visibility.occluded)
      continue;

    auto entity = UraEntityRegistry::from_id(item->id);
    double sx, sy;

    if (item->role == SceneRole::Toplevel) {
      auto* toplevel = server->entities->toplevel(entity);
      if (!toplevel || !toplevel->mapped())
        continue;

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
          lx - item->transform.x - geometry.x,
          ly - item->transform.y - geometry.y,
          &sx,
          &sy
        );
        if (surface)
          return make_client(surface, sx, sy);
      }

      auto* xdg_surface = toplevel->xdg_toplevel->base;
      auto* surface = wlr_xdg_surface_surface_at(
        xdg_surface,
        lx - item->transform.x + xdg_surface->geometry.x,
        ly - item->transform.y + xdg_surface->geometry.y,
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
        lx - item->transform.x,
        ly - item->transform.y,
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
