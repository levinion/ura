#pragma once

#include "ura/core/entity.hpp"
#include "ura/scene/components.hpp"
#include "ura/ura.hpp"
#include <string>

namespace ura {

class UraOutput;
class UraScene;

class UraLayerShell {
public:
  entt::entity entity { entt::null };
  wlr_layer_surface_v1* layer_surface;
  std::string output;
  zwlr_layer_shell_v1_layer layer;
  uint64_t lru;

  const WindowLayout& layout();
  const SurfaceContent& content();
  const SceneNode& node();
  const Visibility& visibility();
  const ZOrder& z_order();

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
  void configure(wlr_box* full_area, wlr_box* usable_area);
  void refresh_visibility(bool output_enabled);
  void set_scene_layer(int z);

private:
  UraScene& scene_model();
  void dismiss_popups();
};

} // namespace ura
