#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ura {

struct Transform {
  int x = 0;
  int y = 0;
  float rotation = 0.f;
  double scale = 1.0;

  bool operator==(const Transform&) const = default;
};

struct Geometry {
  int width = 0;
  int height = 0;

  bool operator==(const Geometry&) const = default;
};

struct Visibility {
  // Set once the role-specific map conditions and tags are satisfied.
  bool visible = false;
  // Filled by the frame planner once occlusion is known.
  bool occluded = false;

  bool operator==(const Visibility&) const = default;
};

struct ZOrder {
  int layer = 0;
  // Monotonic sibling order. This is not an output index; the frame planner
  // resolves it per output.
  uint64_t order = 0;

  bool operator==(const ZOrder&) const = default;
};

// Stable reference to a surface. The main-thread bridge resolves this to a
// wlr_surface; snapshots and worker threads never carry raw wlroots pointers.
struct SurfaceHandle {
  uint64_t id = 0;
  uint32_t generation = 0;

  bool operator==(const SurfaceHandle&) const = default;
};

struct SurfaceRef {
  SurfaceHandle handle;
};

struct WindowTags {
  std::vector<std::string> values;

  bool contains(const std::string& tag) const {
    return std::ranges::find(this->values, tag) != this->values.end();
  }

  auto begin() {
    return this->values.begin();
  }

  auto end() {
    return this->values.end();
  }

  auto begin() const {
    return this->values.begin();
  }

  auto end() const {
    return this->values.end();
  }
};

struct Opacity {
  float value = 1.f;
};

struct ShaderEffect {
  // Renderer-side resource handle. The ECS never owns a GPU object.
  uint64_t resource = 0;
};

struct Animation {
  bool running = false;
  float progress = 0.f;
};

enum class DirtyFlag : uint32_t {
  None = 0,
  Transform = 1u << 0,
  Geometry = 1u << 1,
  Visibility = 1u << 2,
  ZOrder = 1u << 3,
  Opacity = 1u << 4,
  Tags = 1u << 5,
  All = (1u << 6) - 1,
};

struct DirtyState {
  uint64_t epoch = 0;
  uint32_t flags = static_cast<uint32_t>(DirtyFlag::None);
};

} // namespace ura
