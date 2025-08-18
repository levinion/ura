#include "ura/view/view.hpp"
#include "ura/view/client.hpp"
#include "ura/seat/cursor.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/server.hpp"
#include "ura/view/workspace.hpp"

namespace ura {

std::unique_ptr<UraView> UraView::init() {
  auto view = std::make_unique<UraView>();
  auto server = UraServer::get_instance();
  view->scene = wlr_scene_create();
  return view;
}

wlr_scene_tree* UraView::get_scene_tree_or_create(int z) {
  if (this->layers.contains(z))
    return this->layers[z];
  // scene_tree does not exist, create it
  auto scene_tree = wlr_scene_tree_create(&this->scene->tree);
  this->layers[z] = scene_tree;
  // reorder layer
  for (auto& layer : this->layers) {
    wlr_scene_node_raise_to_top(&layer.second->node);
  }
  return scene_tree;
}

UraOutput* UraView::current_output() {
  auto server = UraServer::get_instance();
  auto pos = server->seat->cursor->position();
  auto output =
    wlr_output_layout_output_at(server->output_layout, pos.x, pos.y);
  return UraOutput::from(output);
}

std::optional<UraClient> UraView::foreground_client(double* sx, double* sy) {
  auto server = UraServer::get_instance();
  auto pos = server->seat->cursor->position();
  auto node = wlr_scene_node_at(&this->scene->tree.node, pos.x, pos.y, sx, sy);
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

UraWorkSpace* UraView::get_named_workspace_or_create(std::string name) {
  if (!this->named_workspaces.contains(name)) {
    auto workspace = UraWorkSpace::init();
    workspace->name = name;
    this->named_workspaces[name] = std::move(workspace);
  }
  return this->named_workspaces[name].get();
}

UraWorkSpace* UraView::get_named_workspace(std::string name) {
  if (!this->named_workspaces.contains(name))
    return nullptr;
  return this->named_workspaces[name].get();
}
} // namespace ura
