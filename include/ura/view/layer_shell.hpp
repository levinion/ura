#pragma once

#include "ura/core/entity.hpp"
#include "ura/ura.hpp"
#include <string>

namespace ura {

class UraOutput;

class UraLayerShell {
public:
  entt::entity entity { entt::null };
  wlr_layer_surface_v1* layer_surface;
  wlr_scene_layer_surface_v1* scene_surface;
  wlr_scene_tree* scene_tree;
  std::string output;
  zwlr_layer_shell_v1_layer layer;
  uint64_t lru;

  void init(wlr_layer_surface_v1* layer_surface);
  static UraLayerShell* from(wlr_surface* surface);
  static UraLayerShell* from(uint64_t id);
  void focus();
  void unfocus();
  void map();
  void unmap();
  void commit();
  void destroy();
  uint64_t id();
  bool focusable();

private:
  void dismiss_popups();
};

} // namespace ura
