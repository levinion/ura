#pragma once

#include <cstdint>
#include <vector>

#include "ura/scene/components.hpp"

namespace ura {

enum class SceneRole {
  Toplevel,
  LayerShell,
  Popup,
  SessionLock,
  Primitive,
};

struct SceneItemSnapshot {
  uint64_t id = 0;
  SceneRole role = SceneRole::Primitive;
  WindowLayout layout;
  SurfaceContent content;
  SceneNode node;
  Visibility visibility;
  ZOrder z_order;
  Opacity opacity;
  Effect effect;
  // Non-zero when this node samples a renderer-owned frozen texture instead
  // of a live client surface tree.
  uint64_t render_target = 0;
};

struct SceneSnapshot {
  uint64_t epoch = 0;
  std::vector<SceneItemSnapshot> items;

  bool empty() const {
    return this->items.empty();
  }
};

} // namespace ura
