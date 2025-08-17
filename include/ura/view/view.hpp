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
  Fullscreen = 50,
  Popup = 60,
  Overlay = 70,
  LockScreen = 80
};

class UraView {
public:
  wlr_scene* scene;
  std::map<int, wlr_scene_tree*> layers;
  static std::unique_ptr<UraView> init();
  wlr_scene_tree* get_scene_tree_or_create(int z);
};
} // namespace ura
