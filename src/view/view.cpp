#include "ura/view/view.hpp"
#include <memory>
#include "ura/core/server.hpp"

namespace ura {

std::unique_ptr<UraView> UraView::init() {
  auto view = std::make_unique<UraView>();
  auto server = UraServer::get_instance();
  view->scene = wlr_scene_create();
  view->create_scene_tree(UraSceneLayer::Background);
  view->create_scene_tree(UraSceneLayer::Bottom);
  view->create_scene_tree(UraSceneLayer::Normal);
  view->create_scene_tree(UraSceneLayer::Floating);
  view->create_scene_tree(UraSceneLayer::Top);
  view->create_scene_tree(UraSceneLayer::Popup);
  view->create_scene_tree(UraSceneLayer::Fullscreen);
  view->create_scene_tree(UraSceneLayer::Overlay);
  view->create_scene_tree(UraSceneLayer::LockScreen);
  view->reorder();
  return view;
}

wlr_scene_tree* UraView::create_scene_tree(int z) {
  auto scene_tree = wlr_scene_tree_create(&this->scene->tree);
  this->layers[z] = scene_tree;
  return scene_tree;
}

wlr_scene_tree* UraView::try_get_scene_tree(int z) {
  if (this->layers.contains(z))
    return this->layers[z];
  return nullptr;
}

void UraView::reorder() {
  for (auto& layer : this->layers) {
    wlr_scene_node_raise_to_top(&layer.second->node);
  }
}
} // namespace ura
