#include "ura/scene/scene.hpp"

#include <algorithm>

namespace ura {

UraScene::UraScene(UraEntityRegistry& entities) : entities(entities) {
  entities.set_destroy_callback([this] { this->invalidate(); });
}

UraScene::~UraScene() {
  this->entities.set_destroy_callback({});
}

void UraScene::attach_toplevel(
  entt::entity entity,
  Transform transform,
  Geometry geometry,
  WindowTags tags,
  ZOrder z_order
) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  if (z_order.order == 0)
    z_order.order = this->next_order++;

  registry.emplace_or_replace<Transform>(entity, transform);
  registry.emplace_or_replace<Geometry>(entity, geometry);
  registry.emplace_or_replace<Visibility>(entity, Visibility {});
  registry.emplace_or_replace<ZOrder>(entity, z_order);
  registry.emplace_or_replace<Opacity>(entity, Opacity {});
  registry.emplace_or_replace<WindowTags>(entity, std::move(tags));
  registry.emplace_or_replace<DirtyState>(
    entity,
    DirtyState {
      .epoch = this->current_epoch + 1,
      .flags = static_cast<uint32_t>(DirtyFlag::All),
    }
  );
  ++this->current_epoch;
  this->notify_dirty();
}

void UraScene::attach_layer_shell(
  entt::entity entity,
  Transform transform,
  Geometry geometry,
  ZOrder z_order
) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  if (z_order.order == 0)
    z_order.order = this->next_order++;

  registry.emplace_or_replace<Transform>(entity, transform);
  registry.emplace_or_replace<Geometry>(entity, geometry);
  registry.emplace_or_replace<Visibility>(entity, Visibility {});
  registry.emplace_or_replace<ZOrder>(entity, z_order);
  registry.emplace_or_replace<Opacity>(entity, Opacity {});
  registry.emplace_or_replace<DirtyState>(
    entity,
    DirtyState {
      .epoch = this->current_epoch + 1,
      .flags = static_cast<uint32_t>(DirtyFlag::All),
    }
  );
  ++this->current_epoch;
  this->notify_dirty();
}

void UraScene::set_transform(entt::entity entity, const Transform& transform) {
  auto* current = this->try_get<Transform>(entity);
  if (!current || *current == transform)
    return;
  *current = transform;
  this->touch(entity, DirtyFlag::Transform);
}

void UraScene::set_geometry(entt::entity entity, const Geometry& geometry) {
  auto* current = this->try_get<Geometry>(entity);
  if (!current || *current == geometry)
    return;
  *current = geometry;
  this->touch(entity, DirtyFlag::Geometry);
}

void UraScene::set_layer(entt::entity entity, int layer) {
  auto* current = this->try_get<ZOrder>(entity);
  if (!current || current->layer == layer)
    return;
  current->layer = layer;
  this->touch(entity, DirtyFlag::ZOrder);
}

void UraScene::set_visible(entt::entity entity, bool visible) {
  auto* current = this->try_get<Visibility>(entity);
  if (!current || current->visible == visible)
    return;
  current->visible = visible;
  this->touch(entity, DirtyFlag::Visibility);
}

void UraScene::set_opacity(entt::entity entity, float opacity) {
  auto* current = this->try_get<Opacity>(entity);
  if (!current)
    return;
  auto next = std::clamp(opacity, 0.f, 1.f);
  if (current->value == next)
    return;
  current->value = next;
  this->touch(entity, DirtyFlag::Opacity);
}

void UraScene::set_tags(entt::entity entity, WindowTags&& tags) {
  auto* current = this->try_get<WindowTags>(entity);
  if (!current)
    return;
  *current = std::move(tags);
  this->touch(entity, DirtyFlag::Tags);
}

void UraScene::raise(entt::entity entity) {
  auto* current = this->try_get<ZOrder>(entity);
  if (!current)
    return;
  current->order = this->next_order++;
  this->touch(entity, DirtyFlag::ZOrder);
}

void UraScene::mark_dirty(entt::entity entity, DirtyFlag flag) {
  this->touch(entity, flag);
}

void UraScene::set_dirty_callback(std::function<void()> callback) {
  this->dirty_callback = std::move(callback);
}

const SceneSnapshot& UraScene::snapshot() const {
  if (!this->snapshot_dirty)
    return this->cached_snapshot;

  auto& result = this->cached_snapshot;
  result.items.clear();
  result.epoch = this->current_epoch;

  auto view = this->entities.raw()
                .view<Transform, Geometry, Visibility, ZOrder, Opacity>();
  for (auto [entity, transform, geometry, visibility, z_order, opacity] :
       view.each()) {
    SceneRole role = SceneRole::Primitive;
    if (this->entities.raw().all_of<ToplevelRef>(entity))
      role = SceneRole::Toplevel;
    else if (this->entities.raw().all_of<LayerShellRef>(entity))
      role = SceneRole::LayerShell;
    else if (this->entities.raw().all_of<PopupRef>(entity))
      role = SceneRole::Popup;

    SurfaceHandle surface;
    if (auto* ref = this->entities.raw().try_get<SurfaceRef>(entity))
      surface = ref->handle;

    result.items.push_back(
      SceneItemSnapshot {
        .id = UraEntityRegistry::to_id(entity),
        .role = role,
        .transform = transform,
        .geometry = geometry,
        .visibility = visibility,
        .z_order = z_order,
        .opacity = opacity,
        .surface = surface,
      }
    );
  }

  std::sort(
    result.items.begin(),
    result.items.end(),
    [](const auto& lhs, const auto& rhs) {
      if (lhs.z_order.layer != rhs.z_order.layer)
        return lhs.z_order.layer < rhs.z_order.layer;
      if (lhs.z_order.order != rhs.z_order.order)
        return lhs.z_order.order < rhs.z_order.order;
      return lhs.id < rhs.id;
    }
  );
  this->snapshot_dirty = false;
  return result;
}

std::optional<uint64_t> UraScene::topmost_at(int x, int y) const {
  auto& current = this->snapshot();
  for (auto item = current.items.rbegin(); item != current.items.rend();
       ++item) {
    if (!item->visibility.visible || item->visibility.occluded)
      continue;
    auto right = item->transform.x + item->geometry.width;
    auto bottom = item->transform.y + item->geometry.height;
    if (
      x >= item->transform.x && x < right && y >= item->transform.y
      && y < bottom
    )
      return item->id;
  }
  return {};
}

void UraScene::touch(entt::entity entity, DirtyFlag flag) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  auto* dirty = registry.try_get<DirtyState>(entity);
  if (!dirty) {
    dirty = &registry.emplace<DirtyState>(entity);
  }
  dirty->epoch = this->current_epoch + 1;
  dirty->flags |= static_cast<uint32_t>(flag);
  ++this->current_epoch;
  this->notify_dirty();
}

void UraScene::invalidate() {
  // Structural changes (entity destruction, explicit invalidation) must move
  // the snapshot version even though no component was touched.
  ++this->current_epoch;
  this->notify_dirty();
}

void UraScene::notify_dirty() {
  this->snapshot_dirty = true;
  if (this->dirty_callback)
    this->dirty_callback();
}

} // namespace ura
