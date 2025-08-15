#include "ura/view/view.hpp"
#include <memory>
#include "ura/core/server.hpp"

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
} // namespace ura
