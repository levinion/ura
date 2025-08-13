#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/output.hpp"

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
  layer_shell->commit();
}

void on_layer_shell_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);
  layer_shell->destroy();
  delete layer_shell;
}

void on_layer_shell_surface_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);
  layer_shell->map();
}

void on_layer_shell_surface_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto layer_shell = server->runtime->fetch<UraLayerShell*>(listener);
  layer_shell->unmap();
}

} // namespace ura
