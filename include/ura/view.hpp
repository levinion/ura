#pragma once

#include <memory>
#include <map>
#include "ura/ura.hpp"

namespace ura {

enum UraSceneLayer {
  Background = 0,
  Bottom = 10,
  Normal = 20,
  Floating = 30,
  Top = 40,
  Popup = 50,
  Fullscreen = 60,
  Overlay = 70,
  LockScreen = 80
};

class UraView {
public:
  wlr_scene* scene;
  std::map<int, wlr_scene_tree*> layers;
  static std::unique_ptr<UraView> init();
  wlr_scene_tree* create_scene_tree(int z);
  wlr_scene_tree* try_get_scene_tree(int z);
  void reorder();
};
} // namespace ura
