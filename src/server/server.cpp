#include "ura/server.hpp"
#include <optional>
#include "ura/client.hpp"
#include "ura/output.hpp"
#include "ura/runtime.hpp"
#include "ura/keyboard.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/layer_shell.hpp"
#include "ura/seat.hpp"
#include "ura/lua.hpp"

namespace ura {

UraServer* UraServer::instance = nullptr;

UraServer* UraServer::get_instance() {
  if (UraServer::instance == nullptr) {
    UraServer::instance = new UraServer {};
  }
  return UraServer::instance;
}

// returns the topmost toplevel under current cursor coordination
std::optional<UraClient> UraServer::foreground_client(double* sx, double* sy) {
  auto pos = this->seat->cursor->position();
  auto node =
    wlr_scene_node_at(&this->view->scene->tree.node, pos.x, pos.y, sx, sy);
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return {};
  }
  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return {};
  }
  return UraClient::from(scene_surface->surface);
}

UraOutput* UraServer::current_output() {
  auto pos = this->seat->cursor->position();
  auto output = wlr_output_layout_output_at(this->output_layout, pos.x, pos.y);
  return UraOutput::from(output);
}

UraKeyboard* UraServer::current_keyboard() {
  auto keyboard = this->seat->seat->keyboard_state.keyboard;
  return UraKeyboard::from(keyboard);
}

void UraServer::terminate() {
  this->quit = true;
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
