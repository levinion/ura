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
  auto output = server->current_output();

  // get the layer scene tree based on layer_shell's type
  auto layer_scene_tree =
    output->get_layer_by_type(layer_surface->pending.layer);
  // create a scene tree for the layer_shell's surface
  auto scene_surface =
    wlr_scene_layer_surface_v1_create(layer_scene_tree, layer_surface);

  this->layer_surface = layer_surface;
  this->scene_surface = scene_surface;
  this->output = output;
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
  auto seat = server->seat->seat;
  auto keyboard = wlr_seat_get_keyboard(seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      seat,
      this->layer_surface->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}

} // namespace ura
