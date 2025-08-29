#pragma once

#include <list>
#include <memory>
#include <map>
#include <unordered_map>
#include "ura/ura.hpp"
#include "ura/view/output.hpp"
#include "ura/view/workspace.hpp"

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
  Vec<UraOutput*> outputs;
  std::unordered_map<std::string, std::unique_ptr<UraWorkSpace>>
    named_workspaces;

  static std::unique_ptr<UraView> init();
  wlr_scene_tree* get_scene_tree_or_create(int z);
  UraWorkSpace* get_named_workspace_or_create(std::string name);
  UraWorkSpace* get_named_workspace(std::string name);
  UraOutput* current_output();
  std::optional<UraClient> foreground_client(double* sx, double* sy);
};
} // namespace ura
