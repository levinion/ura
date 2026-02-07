#include "ura/view/view.hpp"
#include <utility>
#include "ura/view/client.hpp"
#include "ura/seat/cursor.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/server.hpp"
#include "ura/view/workspace.hpp"

namespace ura {

std::unique_ptr<UraView> UraView::init() {
  auto view = std::make_unique<UraView>();
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
  if (!output)
    return nullptr;
  return UraOutput::from(output);
}

UraOutput* UraView::get_output_by_name(std::string_view name) {
  // TODO: this should be removed when unordered_map supports string_view as a read key
  auto key = std::string(name);
  return this->outputs.contains(key) ? this->outputs[key] : nullptr;
}

std::optional<UraClient> UraView::foreground_client() {
  auto server = UraServer::get_instance();
  auto pos = server->seat->cursor->position();
  double sx, sy;
  auto node =
    wlr_scene_node_at(&this->scene->tree.node, pos.x, pos.y, &sx, &sy);
  if (!node || node->type != WLR_SCENE_NODE_BUFFER) {
    return {};
  }
  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface) {
    return {};
  }
  auto client = UraClient::from(scene_surface->surface);
  client.sx = sx;
  client.sy = sy;
  return client;
}

wlr_scene_tree* UraView::get_layer_by_type(zwlr_layer_shell_v1_layer type) {
  wlr_scene_tree* layer;
  auto server = UraServer::get_instance();
  switch (type) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Background);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Bottom);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Overlay);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Top);
      break;
    default:
      std::unreachable();
  }
  return layer;
}

void UraView::notify_scale(wlr_surface* surface, double scale) {
  wlr_fractional_scale_v1_notify_scale(surface, scale);
  wlr_surface_set_preferred_buffer_scale(surface, ceil(scale));
}

} // namespace ura
