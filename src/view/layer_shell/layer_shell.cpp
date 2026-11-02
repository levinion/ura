#include <algorithm>
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/ura.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/seat/seat.hpp"

namespace ura {

UraScene& UraLayerShell::scene_model() {
  return *UraServer::get_instance()->view->scene;
}

const Transform& UraLayerShell::transform() {
  return this->scene_model().get<Transform>(this->entity);
}

const Geometry& UraLayerShell::geometry() {
  return this->scene_model().get<Geometry>(this->entity);
}

const Visibility& UraLayerShell::visibility() {
  return this->scene_model().get<Visibility>(this->entity);
}

const ZOrder& UraLayerShell::z_order() {
  return this->scene_model().get<ZOrder>(this->entity);
}

void UraLayerShell::set_scene_layer(int z) {
  this->scene_model().set_layer(this->entity, z);
  this->scene_model().raise(this->entity);
}

void UraLayerShell::refresh_visibility(bool output_enabled) {
  auto visible = output_enabled && this->layer_surface->surface->mapped;
  this->scene_model().set_visible(this->entity, visible);
}

void UraLayerShell::configure(wlr_box* full_area, wlr_box* usable_area) {
  auto* state = &this->layer_surface->current;

  auto bounds = state->exclusive_zone == -1 ? *full_area : *usable_area;
  wlr_box box = {
    .x = 0,
    .y = 0,
    .width = static_cast<int>(state->desired_width),
    .height = static_cast<int>(state->desired_height),
  };

  if (box.width == 0) {
    box.x = bounds.x + state->margin.left;
    box.width = bounds.width - (state->margin.left + state->margin.right);
  } else if (
    state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
    && state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
  ) {
    box.x = bounds.x + bounds.width / 2 - box.width / 2;
  } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) {
    box.x = bounds.x + state->margin.left;
  } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) {
    box.x = bounds.x + bounds.width - box.width - state->margin.right;
  } else {
    box.x = bounds.x + bounds.width / 2 - box.width / 2;
  }

  if (box.height == 0) {
    box.y = bounds.y + state->margin.top;
    box.height = bounds.height - (state->margin.top + state->margin.bottom);
  } else if (
    state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
    && state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
  ) {
    box.y = bounds.y + bounds.height / 2 - box.height / 2;
  } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) {
    box.y = bounds.y + state->margin.top;
  } else if (state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) {
    box.y = bounds.y + bounds.height - box.height - state->margin.bottom;
  } else {
    box.y = bounds.y + bounds.height / 2 - box.height / 2;
  }

  this->scene_model().set_transform(
    this->entity,
    Transform { box.x, box.y, 0.f, 1.0 }
  );
  this->scene_model().set_geometry(
    this->entity,
    Geometry { box.width, box.height }
  );
  wlr_layer_surface_v1_configure(this->layer_surface, box.width, box.height);

  if (this->layer_surface->surface->mapped && state->exclusive_zone > 0) {
    auto edge = wlr_layer_surface_v1_get_exclusive_edge(this->layer_surface);
    switch (edge) {
      case WLR_EDGE_NONE:
        break;
      case WLR_EDGE_TOP:
        usable_area->y += state->exclusive_zone + state->margin.top;
        usable_area->height -= state->exclusive_zone + state->margin.top;
        break;
      case WLR_EDGE_BOTTOM:
        usable_area->height -= state->exclusive_zone + state->margin.bottom;
        break;
      case WLR_EDGE_LEFT:
        usable_area->x += state->exclusive_zone + state->margin.left;
        usable_area->width -= state->exclusive_zone + state->margin.left;
        break;
      case WLR_EDGE_RIGHT:
        usable_area->width -= state->exclusive_zone + state->margin.right;
        break;
    }
    usable_area->width = std::max(usable_area->width, 0);
    usable_area->height = std::max(usable_area->height, 0);
  }
}

void UraLayerShell::init(wlr_layer_surface_v1* layer_surface) {
  auto server = UraServer::get_instance();
  // if no output given, then assign current output to surface
  if (!layer_surface->output) {
    auto output = server->view->current_output();
    layer_surface->output = output->output;
  }
  auto output = UraOutput::from(layer_surface->output);
  this->output = output->name;

  this->layer = layer_surface->pending.layer;
  this->layer_surface = layer_surface;
  layer_surface->surface->data = this;
  this->entity = server->entities->create_layer_shell(this);
  server->view->scene->attach_layer_shell(
    this->entity,
    Transform {},
    Geometry {},
    ZOrder {
      server->view->get_z_index_by_type(this->layer),
      0,
    }
  );
  server->view->notify_scale(
    layer_surface->surface,
    layer_surface->output->scale
  );

  server->runtime->register_callback(
    &layer_surface->surface->events.commit,
    on_layer_shell_surface_commit,
    this
  );
  server->runtime->register_callback(
    &layer_surface->surface->events.map,
    on_layer_shell_surface_map,
    this
  );
  server->runtime->register_callback(
    &layer_surface->surface->events.unmap,
    on_layer_shell_surface_unmap,
    this
  );
  server->runtime->register_callback(
    &layer_surface->events.destroy,
    on_layer_shell_surface_destroy,
    this
  );

  // add this shell to output's layer
  auto& list = output->layer_shells_from_layer(layer_surface->pending.layer);
  list.push_back(this);
  server->view->layer_shell_surfaces.push_back(this);
}

UraLayerShell* UraLayerShell::from(wlr_surface* surface) {
  return static_cast<UraLayerShell*>(surface->data);
}

UraLayerShell* UraLayerShell::from(uint64_t id) {
  return UraServer::get_instance()->entities->layer_shell(id);
}

void UraLayerShell::focus() {
  auto server = UraServer::get_instance();
  auto keyboard = wlr_seat_get_keyboard(server->seat->seat);
  if (keyboard)
    wlr_seat_keyboard_notify_enter(
      server->seat->seat,
      this->layer_surface->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  this->lru = std::chrono::steady_clock::now().time_since_epoch().count();
}

void UraLayerShell::unfocus() {
  this->dismiss_popups();
}

void UraLayerShell::map() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  auto geometry = output ? output->logical_geometry() : Vec4<int> {};
  if (!output || geometry.width <= 0 || geometry.height <= 0) {
    this->refresh_visibility(false);
    return;
  }
  this->refresh_visibility(true);
  output->configure_layers();
}

void UraLayerShell::unmap() {
  this->refresh_visibility(false);
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  if (output)
    output->configure_layers();
}

void UraLayerShell::commit() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  if (!output)
    return;
  auto layer_changed = false;
  if (
    this->layer_surface->initialized
    && this->layer_surface->current.committed & WLR_LAYER_SURFACE_V1_STATE_LAYER
  ) {
    // put the surface under proper layer
    output->layer_shells_from_layer(this->layer).remove(this);
    this->layer = this->layer_surface->current.layer;
    output->layer_shells_from_layer(this->layer).push_back(this);
    this->set_scene_layer(server->view->get_z_index_by_type(this->layer));
    layer_changed = true;
  }

  auto output_geo = output->logical_geometry();
  if (output_geo.width <= 0 || output_geo.height <= 0)
    return;

  // configure size
  auto width = this->layer_surface->current.desired_width;
  auto height = this->layer_surface->current.desired_height;
  if (width == 0)
    width = output_geo.width;
  if (height == 0)
    height = output_geo.height;

  if (
    layer_changed || width != this->layer_surface->current.actual_width
    || height != this->layer_surface->current.actual_height
  ) {
    if (width <= 0 || height <= 0)
      return;
    output->configure_layers();
  }

  if (this->layer_surface->initial_commit && this->focusable()) {
    server->seat->focus(this);
  }
}

void UraLayerShell::destroy() {
  auto server = UraServer::get_instance();
  server->runtime->remove(this);
  server->view->layer_shell_surfaces.remove(this);
  auto output = server->view->get_output_by_name(this->output);
  if (output) {
    auto& layer = output->layer_shells_from_layer(this->layer);
    layer.remove(this);
    if (this->focusable())
      server->seat->focus_lru();
  }
  this->dismiss_popups();
  if (output)
    output->configure_layers();

  server->entities->destroy(this->entity);
  this->entity = entt::null;
  server->view->schedule_frames();
}

void UraLayerShell::dismiss_popups() {
  wlr_xdg_popup *_popup, *tmp;
  wl_list_for_each_safe(_popup, tmp, &this->layer_surface->popups, link) {
    wlr_xdg_popup_destroy(_popup);
  }
}

uint64_t UraLayerShell::id() {
  return UraEntityRegistry::to_id(this->entity);
}

bool UraLayerShell::focusable() {
  return this->layer_surface->current.keyboard_interactive
    != ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
}

} // namespace ura
