#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "ura/core/entity.hpp"
#include "ura/scene/components.hpp"
#include "ura/scene/snapshot.hpp"

namespace ura {

class UraScene {
public:
  explicit UraScene(UraEntityRegistry& entities);
  ~UraScene();

  void attach_toplevel(
    entt::entity entity,
    Rect<double> target,
    WindowTags tags,
    ZOrder z_order
  );

  void attach_layer_shell(entt::entity entity, Rect<double> target, ZOrder z_order);
  void attach_session_lock(entt::entity entity, Rect<double> bounds);

  void set_layout_target(entt::entity entity, const Rect<double>& target);
  void set_surface_content(entt::entity entity, const SurfaceContent& content);
  void set_node_bounds(entt::entity entity, const Rect<double>& bounds);
  // Reset SceneNode.bounds to the resting rectangle derived from WindowLayout
  // and SurfaceContent. Use this for layer shells and snap-to-target cases.
  void sync_scene_node(entt::entity entity);
  Rect<double> window_bounds(entt::entity entity) const;
  Rect<double> node_bounds(entt::entity entity) const;
  WindowTransition& ensure_window_transition(entt::entity entity);
  WindowAnimation& ensure_window_animation(entt::entity entity);
  void set_layer(entt::entity entity, int layer);
  void set_visible(entt::entity entity, bool visible);
  void set_opacity(entt::entity entity, float opacity);
  void set_effect(entt::entity entity, const Effect& effect);
  void set_tags(entt::entity entity, WindowTags&& tags);
  void raise(entt::entity entity);

  void mark_dirty(entt::entity entity, DirtyFlag flag);

  template<typename T>
  T& get(entt::entity entity) {
    return this->entities.raw().get<T>(entity);
  }

  template<typename T>
  const T& get(entt::entity entity) const {
    return this->entities.raw().get<T>(entity);
  }

  template<typename T>
  T* try_get(entt::entity entity) {
    return this->entities.raw().try_get<T>(entity);
  }

  template<typename T>
  const T* try_get(entt::entity entity) const {
    return this->entities.raw().try_get<T>(entity);
  }

  const SceneSnapshot& snapshot() const;
  void invalidate();
  void destroy(entt::entity entity);
  void set_dirty_callback(std::function<void()> callback);
  // SceneNode.bounds changes are the animation hot path. The compositor uses
  // this callback to damage only outputs touched by the old and new rectangle.
  void set_visual_dirty_callback(
    std::function<void(entt::entity, const Rect<double>&, const Rect<double>&)> callback
  );

  entt::registry& raw();
  const entt::registry& raw() const;

  std::optional<uint64_t> topmost_at(int x, int y) const;

  uint64_t epoch() const {
    return this->current_epoch;
  }

private:
  UraEntityRegistry& entities;
  uint64_t current_epoch = 1;
  uint64_t next_order = 1;
  std::function<void()> dirty_callback;
  std::function<void(entt::entity, const Rect<double>&, const Rect<double>&)>
    visual_dirty_callback;
  mutable SceneSnapshot cached_snapshot;
  mutable bool snapshot_dirty = true;

  void touch(entt::entity entity, DirtyFlag flag, bool notify = true);
  void notify_dirty();
  void notify_visual_dirty(
    entt::entity entity,
    const Rect<double>& previous,
    const Rect<double>& current
  );
};

} // namespace ura
