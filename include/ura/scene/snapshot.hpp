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
  Transform transform;
  Geometry geometry;
  Visibility visibility;
  ZOrder z_order;
  Opacity opacity;
  SurfaceHandle surface;
};

struct SceneSnapshot {
  uint64_t epoch = 0;
  std::vector<SceneItemSnapshot> items;

  bool empty() const {
    return this->items.empty();
  }
};

} // namespace ura
