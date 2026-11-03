#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "ura/render/effect.hpp"
#include "ura/scene/animation.hpp"
#include "ura/util/rect.hpp"

namespace ura {

// Stable reference to client content. It is kept separate from the compositor
// window so content lifetime and window lifetime can diverge later (for
// example while a closing animation keeps the last frame alive).
struct SurfaceHandle {
  uint64_t id = 0;
  uint32_t generation = 0;

  bool operator==(const SurfaceHandle&) const = default;
};

// Immutable description of the content currently committed by the client.
// x/y is the origin of the xdg window geometry inside the root surface; CSD
// shadows and subsurfaces live outside this rectangle. The frame world still
// resolves the actual texture through the role-specific live object because
// snapshots must never carry raw wlroots pointers.
struct SurfaceContent {
  Rect<int> geometry;

  bool empty() const {
    return this->geometry.empty();
  }

  bool operator==(const SurfaceContent&) const = default;
};

// What the layout wants the window to become. This is a request, not the
// current on-screen state. Resizing the client is only one possible way to
// reach it.
struct WindowLayout {
  Rect<double> target;

  bool operator==(const WindowLayout&) const = default;

  Rect<double> resolve(const SurfaceContent& content) const;
};

// The compositor's real window object. `bounds` is the rectangle currently
// sampled from SurfaceContent and drawn on screen. It may temporarily differ
// from both the surface content size and the layout target while a transition
// is running.
struct SceneNode {
  Rect<double> bounds;
  // Radians, around the center of `bounds`.
  float rotation = 0.f;

  bool operator==(const SceneNode&) const = default;
};

// Resolve the resting bounds for a window: the layout owns the anchor, while
// the committed content owns the effective size.
inline Rect<double> WindowLayout::resolve(const SurfaceContent& content) const {
  auto width = content.geometry.width > 0
    ? static_cast<double>(content.geometry.width)
    : this->target.width;
  auto height = content.geometry.height > 0
    ? static_cast<double>(content.geometry.height)
    : this->target.height;
  return Rect<double> {
    .x = this->target.x,
    .y = this->target.y,
    .width = width,
    .height = height,
  };
}

// Optional renderer-side offscreen resource. Its presence means the SceneNode
// must be composed through a RenderTarget (multi-pass effect, frozen content,
// closing animation, ...). Normal windows sample the committed client texture
// directly and never allocate one.
struct RenderTarget {
  // Renderer resource handle. The ECS never owns the GPU object itself.
  uint64_t resource = 0;

  bool operator==(const RenderTarget&) const = default;
};

struct ShaderEffect {
  EffectInstance effect;
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

  bool operator==(const Opacity&) const = default;
};

enum class WindowTransitionPhase {
  Idle,
  WaitingForCommit,
  Animating,
};

// Commit-first resize transition. SceneNode.bounds remains at `from` until a
// matching configure has been acknowledged and a later surface commit arrived.
// A timeout may resolve the transition using the last valid content geometry
// without stretching stale content.
struct WindowTransition {
  WindowTransitionPhase phase = WindowTransitionPhase::Idle;

  Rect<double> from;
  Rect<double> requested_target;
  Rect<double> resolved_target;

  uint64_t generation = 0;
  uint32_t configure_serial = 0;
  uint32_t base_configure_serial = 0;
  uint64_t base_seq = 0;
  AnimationTimePoint deadline {};
  AnimationOptions options;

  void inherit(const AnimationOptions& candidate) {
    if (candidate.duration > this->options.duration)
      this->options = candidate;
  }
};

// Time-based animation of SceneNode.bounds. The same component is used for
// moves, resize transitions and opening effects. It never owns a texture or a
// wlroots resource; the frame world samples the current client texture.
struct WindowAnimation {
  bool running = false;
  Rect<double> from;
  Rect<double> to;

  AnimationTimePoint start {};
  AnimationOptions options;
  uint64_t generation = 0;

  float from_opacity = 1.f;
  float to_opacity = 1.f;
  float from_rotation = 0.f;
  float to_rotation = 0.f;
  // Set by settle() so a layout retarget or commit-first transition can carry
  // the opacity part of an opening/closing animation across the pause.
  bool carry_opacity = false;
  bool carry_rotation = false;
  // Used by detached close-animation nodes. The animation system defers
  // destruction by one frame so the final visual state is presented first.
  bool destroy_on_finish = false;
  bool destroy_pending = false;
};

// One scalar/vector uniform ramp. Tracks live outside WindowAnimation so the
// layout animation and shader parameter animation remain independent.
struct EffectUniformTrack {
  std::string name;
  // Index into EffectInstance::uniforms, resolved when the track is created.
  std::size_t slot = static_cast<std::size_t>(-1);
  EffectUniformType type = EffectUniformType::Float;
  std::array<float, 4> from {};
  std::array<float, 4> to {};
  AnimationTimePoint start {};
  AnimationOptions options;
  bool running = true;
};

struct EffectUniformAnimation {
  std::vector<EffectUniformTrack> tracks;
};

enum class DirtyFlag : uint32_t {
  None = 0,
  Layout = 1u << 0,
  Content = 1u << 1,
  Visibility = 1u << 2,
  ZOrder = 1u << 3,
  Opacity = 1u << 4,
  Tags = 1u << 5,
  Visual = 1u << 6,
  Effect = 1u << 7,
  All = (1u << 8) - 1,
};

struct DirtyState {
  uint64_t epoch = 0;
  uint32_t flags = static_cast<uint32_t>(DirtyFlag::None);
};

} // namespace ura
