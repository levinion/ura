#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"
#include "ura/callback.hpp"
#include "ura/seat.hpp"

namespace ura {

void UraLayerShell::init(wlr_layer_surface_v1* layer_surface) {
  auto server = UraServer::get_instance();
  // if no output given, then assign current output to surface
  if (!layer_surface->output) {
    auto output = server->current_output();
    layer_surface->output = output->output;
  }
  this->output = UraOutput::from(layer_surface->output);

  // get the layer scene tree based on layer_shell's type
  auto layer_scene_tree =
    output->get_layer_by_type(layer_surface->pending.layer);
  // create a scene tree for the layer_shell's surface
  auto scene_surface =
    wlr_scene_layer_surface_v1_create(layer_scene_tree, layer_surface);

  this->layer_surface = layer_surface;
  this->scene_surface = scene_surface;
  this->scene_tree = scene_surface->tree;
  layer_surface->surface->data = this;

  // notify scale
  wlr_fractional_scale_v1_notify_scale(
    layer_surface->surface,
    layer_surface->output->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    layer_surface->surface,
    layer_surface->output->scale
  );

  server->runtime->register_callback(
    &layer_surface->surface->events.commit,
    on_layer_shell_surface_commit,
    this
  );
  server->runtime->register_callback(
    &layer_surface->surface->events.destroy,
    on_layer_shell_surface_destroy,
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

  // add this shell to output's layer
  auto& list =
    this->output->get_layer_list_by_type(layer_surface->pending.layer);
  list.push_back(this);
}

UraLayerShell* UraLayerShell::from(wlr_surface* surface) {
  return static_cast<UraLayerShell*>(surface->data);
}

void UraLayerShell::focus() {
  auto server = UraServer::get_instance();
  auto keyboard = wlr_seat_get_keyboard(server->seat->seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      server->seat->seat,
      this->layer_surface->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}

void UraLayerShell::map() {
  wlr_scene_node_set_enabled(&this->scene_surface->tree->node, true);
  this->output->configure_layers();
}

void UraLayerShell::unmap() {
  wlr_scene_node_set_enabled(&this->scene_surface->tree->node, false);
  this->output->configure_layers();
}

void UraLayerShell::commit() {
  auto server = UraServer::get_instance();
  auto output = this->output;
  if (this->layer_surface->initialized
      && this->layer_surface->current.committed
        & WLR_LAYER_SURFACE_V1_STATE_LAYER) {
    auto layer_type = this->layer_surface->current.layer;
    auto layer = output->get_layer_by_type(layer_type);
    // put the surface under proper layer
    wlr_scene_node_reparent(&this->scene_tree->node, layer);
  }

  // configure size
  auto width = this->layer_surface->pending.desired_width;
  auto height = this->layer_surface->pending.desired_height;

  auto output_geo = output->logical_geometry();
  if (width == 0)
    width = output_geo.width;
  if (height == 0)
    height = output_geo.height;

  if (width != this->layer_surface->current.actual_width
      || height != this->layer_surface->current.actual_height) {
    wlr_layer_surface_v1_configure(this->layer_surface, width, height);
    if (output->configure_layers()) {
      for (auto toplevel : this->output->current_workspace->toplevels)
        toplevel->request_commit();
    }
  }

  if (this->layer_surface->initial_commit
      && this->layer_surface->current.keyboard_interactive
        != ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE) {
    server->seat->focus(this);
  }
}

void UraLayerShell::destroy() {
  auto server = UraServer::get_instance();
  server->runtime->remove(this);
  // remove from output's layer
  auto& layer =
    this->output->get_layer_list_by_type(this->layer_surface->pending.layer);
  layer.remove(this);
  auto toplevel = this->output->current_workspace->focus_stack.top();
  if (toplevel
      && this->layer_surface->current.keyboard_interactive
        != ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE)
    server->seat->focus(toplevel.value());
}

} // namespace ura
