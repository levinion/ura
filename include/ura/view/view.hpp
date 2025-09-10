#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include "ura/ura.hpp"
#include "ura/view/output.hpp"
#include "ura/view/workspace.hpp"

namespace ura {

enum UraSceneLayer {
  Clear = -50,
  Background = 0,
  Bottom = 50,
  Normal = 100,
  Floating = 150,
  Top = 200,
  Fullscreen = 250,
  Popup = 300,
  Overlay = 350,
  LockScreen = 400
};

class UraView {
public:
  wlr_scene* scene;
  std::map<int, wlr_scene_tree*> layers;
  std::unordered_map<std::string, UraOutput*> outputs;
  Vec<std::unique_ptr<UraWorkSpace>> workspaces;
  std::unordered_map<std::string, Vec<UraWorkSpace*>> indexed_workspaces;
  std::unordered_map<std::string, UraWorkSpace*> named_workspaces;

  static std::unique_ptr<UraView> init();
  wlr_scene_tree* get_scene_tree_or_create(int z);
  UraWorkSpace* get_named_workspace_or_create(std::string name);
  UraWorkSpace* get_named_workspace(std::string name);
  UraOutput* current_output();
  UraOutput* get_output_by_name(std::string& name);
  std::optional<UraClient> foreground_client(double* sx, double* sy);
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
  void notify_scale(wlr_surface* surface, float scale);
};
} // namespace ura
