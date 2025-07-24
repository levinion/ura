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
  auto layer_shell = new UraLayerShell {};
  layer_shell->init(layer_surface);
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
    wlr_scene_node_reparent(&layer_shell->scene_tree->node, layer);
  }

  // configure size
  auto width = layer_shell->layer_surface->pending.desired_width;
  auto height = layer_shell->layer_surface->pending.desired_height;
  auto scale = output->output->scale;

  if (width == 0) {
    width = output->output->current_mode->width / scale;
  }
  if (height == 0) {
    height = output->output->current_mode->height / scale;
  }

  if (width != layer_shell->layer_surface->current.actual_width
      || height != layer_shell->layer_surface->current.actual_height) {
    wlr_layer_surface_v1_configure(layer_shell->layer_surface, width, height);
    output->configure_layers();
  }
}

void on_layer_shell_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);

  // this will destroy scene node so there's no need to destroy again
  // wlr_layer_surface_v1_destroy(layer_shell->layer_surface);

  server->runtime->remove(layer_shell);
  // remove from output's layer
  auto& list = layer_shell->output->get_layer_list_by_type(
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

} // namespace ura
