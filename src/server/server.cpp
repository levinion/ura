
#include "ura/server.hpp"
#include "ura/config.hpp"
#include "ura/output.hpp"
#include "ura/runtime.hpp"
#include "ura/keyboard.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/layer_shell.hpp"

namespace ura {

UraServer* UraServer::instance = nullptr;

UraServer* UraServer::get_instance() {
  if (UraServer::instance == nullptr) {
    UraServer::instance = new UraServer {};
  }
  return UraServer::instance;
}

// returns the topmost toplevel under current cursor coordination
UraToplevel* UraServer::foreground_toplevel(double* sx, double* sy) {
  auto node = wlr_scene_node_at(
    &this->scene->tree.node,
    this->cursor->x,
    this->cursor->y,
    sx,
    sy
  );
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return nullptr;
  }
  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return nullptr;
  }
  auto toplevel = wlr_xdg_toplevel_try_from_wlr_surface(scene_surface->surface);
  if (!toplevel)
    return nullptr;
  return UraToplevel::from(toplevel->base->surface);
}

UraLayerShell* UraServer::foreground_layer_shell(double* sx, double* sy) {
  auto node = wlr_scene_node_at(
    &this->scene->tree.node,
    this->cursor->x,
    this->cursor->y,
    sx,
    sy
  );
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return nullptr;
  }
  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return nullptr;
  }
  auto layer_surface =
    wlr_layer_surface_v1_try_from_wlr_surface(scene_surface->surface);
  if (!layer_surface)
    return nullptr;
  return UraLayerShell::from(layer_surface->surface);
}

wlr_xdg_popup* UraServer::foreground_popup(double* sx, double* sy) {
  auto node = wlr_scene_node_at(
    &this->scene->tree.node,
    this->cursor->x,
    this->cursor->y,
    sx,
    sy
  );
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return nullptr;
  }
  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return nullptr;
  }
  auto popup = wlr_xdg_popup_try_from_wlr_surface(scene_surface->surface);
  if (!popup)
    return nullptr;
  return popup;
}

UraOutput* UraServer::current_output() {
  auto output = wlr_output_layout_output_at(
    this->output_layout,
    this->cursor->x,
    this->cursor->y
  );
  return UraOutput::from(output);
}

UraKeyboard* UraServer::current_keyboard() {
  auto keyboard = this->seat->keyboard_state.keyboard;
  return UraKeyboard::from(keyboard);
}

void UraServer::terminate() {
  for (auto output : this->runtime->outputs)
    for (auto& workspace : output->workspaces)
      for (auto toplevel : workspace->toplevels) toplevel->close();

  wl_display_flush_clients(this->display);
  wl_display_terminate(this->display);
}

/* Ouput Manager */
void UraServer::update_output_configuration() {
  auto configuration = wlr_output_configuration_v1_create();
  for (auto output : this->runtime->outputs) {
    wlr_output_configuration_head_v1_create(configuration, output->output);
  }
  wlr_output_manager_v1_set_configuration(this->output_manager, configuration);
}
} // namespace ura
