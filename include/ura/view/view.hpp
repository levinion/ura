#pragma once

#include <memory>
#include <map>
#include "ura/ura.hpp"
#include "ura/view/output.hpp"

namespace ura {

enum UraSceneLayer {
  Clear = -50,
  Background = 0,
  Bottom = 50,
  Normal = 100,
  Top = 150,
  Floating = 200,
  Fullscreen = 250,
  Popup = 300,
  Overlay = 350,
  LockScreen = 400
};

class UraView {
public:
  wlr_scene* scene;
  std::map<int, wlr_scene_tree*> layers;
  absl::flat_hash_map<std::string, UraOutput*> outputs;
  Vec<UraToplevel*> toplevels;

  static std::unique_ptr<UraView> init();
  wlr_scene_tree* get_scene_tree_or_create(int z);
  UraOutput* current_output();
  UraOutput* get_output_by_name(std::string_view name);
  std::optional<UraClient> foreground_client();
  wlr_scene_tree* get_layer_by_type(zwlr_layer_shell_v1_layer type);
  void notify_scale(wlr_surface* surface, double scale);
};
} // namespace ura
