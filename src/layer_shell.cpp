#include "ura/layer_shell.hpp"
#include "ura/callback.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"

namespace ura {
void on_layer_shell_new_surface(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_surface = static_cast<wlr_layer_surface_v1*>(data);

  // if no output given, then assign current output to surface
  if (!layer_surface->output) {
    auto output = server->current_output();
    layer_surface->output = output->output;
  }

  auto outputs = server->runtime->outputs;
  auto output = *std::find_if(outputs.begin(), outputs.end(), [&](auto i) {
    return i->output == layer_surface->output;
  });

  auto scene_tree = output->get_layer_by_type(layer_surface->pending.layer);
  auto scene_surface =
    wlr_scene_layer_surface_v1_create(scene_tree, layer_surface);

  // notify scale
  wlr_fractional_scale_v1_notify_scale(
    layer_surface->surface,
    layer_surface->output->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    layer_surface->surface,
    layer_surface->output->scale
  );

  auto layer_shell = new UraLayerShell {};
  layer_shell->layer_surface = layer_surface;
  layer_shell->scene_surface = scene_surface;
  layer_shell->output = output;

  server->runtime->register_callback(
    &layer_surface->surface->events.commit,
    on_layer_shell_surface_commit,
    layer_shell
  );

  server->runtime->register_callback(
    &layer_surface->surface->events.destroy,
    on_layer_shell_surface_destroy,
    layer_shell
  );

  server->runtime->register_callback(
    &layer_surface->surface->events.map,
    on_layer_shell_surface_map,
    layer_shell
  );

  server->runtime->register_callback(
    &layer_surface->surface->events.unmap,
    on_layer_shell_surface_unmap,
    layer_shell
  );

  // server->runtime->register_callback(
  //   &layer_surface->events.new_popup,
  //   on_layer_shell_new_popup,
  //   layer_shell
  // );

  // add this shell to output's layer
  auto list =
    layer_shell->output->get_layer_list_by_type(layer_surface->pending.layer);
  list.push_back(layer_shell);
}

void on_layer_shell_surface_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);

  auto output = layer_shell->output;

  if (layer_shell->layer_surface->initialized
      && layer_shell->layer_surface->current.committed
        & WLR_LAYER_SURFACE_V1_STATE_LAYER) {
    auto layer_type = layer_shell->layer_surface->current.layer;
    auto layer = output->get_layer_by_type(layer_type);
    // put the surface under proper layer
    wlr_scene_node_reparent(&layer_shell->scene_surface->tree->node, layer);
  }

  // TODO: proper layer size
  auto width = output->output->current_mode->width;
  auto height = output->output->current_mode->height;
  wlr_layer_surface_v1_configure(layer_shell->layer_surface, width, height);
  output->configure_layers();
}

void on_layer_shell_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);

  // this will destroy scene node so there's no need to destroy again
  // wlr_layer_surface_v1_destroy(layer_shell->layer_surface);

  server->runtime->remove(layer_shell);
  // remove from output's layer
  auto list = layer_shell->output->get_layer_list_by_type(
    layer_shell->layer_surface->pending.layer
  );
  list.remove(layer_shell);
  delete layer_shell;
}

void on_layer_shell_surface_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);
  wlr_scene_node_set_enabled(&layer_shell->scene_surface->tree->node, true);
  auto output = layer_shell->output;
  output->configure_layers();
}

void on_layer_shell_surface_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);
  wlr_scene_node_set_enabled(&layer_shell->scene_surface->tree->node, false);
  auto output = layer_shell->output;
  output->configure_layers();
}

void on_layer_shell_new_popup(wl_listener* listener, void* data) {}

} // namespace ura
