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

  void attach_toplevel(
    entt::entity entity,
    Transform transform,
    Geometry geometry,
    WindowTags tags,
    ZOrder z_order
  );

  void attach_layer_shell(
    entt::entity entity,
    Transform transform,
    Geometry geometry,
    ZOrder z_order
  );

  void set_transform(entt::entity entity, const Transform& transform);
  void set_geometry(entt::entity entity, const Geometry& geometry);
  void set_layer(entt::entity entity, int layer);
  void set_visible(entt::entity entity, bool visible);
  void set_opacity(entt::entity entity, float opacity);
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

  SceneSnapshot snapshot() const;
  void set_dirty_callback(std::function<void()> callback);

  std::optional<uint64_t> topmost_at(int x, int y) const;

  uint64_t epoch() const {
    return this->current_epoch;
  }

private:
  UraEntityRegistry& entities;
  uint64_t current_epoch = 1;
  uint64_t next_order = 1;
  std::function<void()> dirty_callback;

  void touch(entt::entity entity, DirtyFlag flag);
  void notify_dirty();
};

} // namespace ura
