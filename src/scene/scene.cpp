#include "ura/scene/scene.hpp"

#include <algorithm>

#include "ura/scene/layer.hpp"

namespace ura {

UraScene::UraScene(UraEntityRegistry& entities) : entities(entities) {
  entities.set_destroy_callback([this] { this->invalidate(); });
}

UraScene::~UraScene() {
  this->entities.set_destroy_callback({});
}

void UraScene::attach_toplevel(
  entt::entity entity,
  Rect<double> target,
  WindowTags tags,
  ZOrder z_order
) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  if (z_order.order == 0)
    z_order.order = this->next_order++;

  WindowLayout layout { .target = target };
  SurfaceContent content {
    .geometry =
      Rect<int> {
        .width = static_cast<int>(target.width),
        .height = static_cast<int>(target.height),
      },
  };
  SceneNode node { .bounds = layout.resolve(content) };

  registry.emplace_or_replace<WindowLayout>(entity, layout);
  registry.emplace_or_replace<SurfaceContent>(entity, content);
  registry.emplace_or_replace<SceneNode>(entity, node);
  registry.emplace_or_replace<Visibility>(entity, Visibility {});
  registry.emplace_or_replace<ZOrder>(entity, z_order);
  registry.emplace_or_replace<Opacity>(entity, Opacity {});
  registry.emplace_or_replace<WindowTags>(entity, std::move(tags));
  registry.emplace_or_replace<WindowTransition>(entity, WindowTransition {});
  registry.emplace_or_replace<WindowAnimation>(entity, WindowAnimation {});
  registry.emplace_or_replace<ShaderEffect>(entity, ShaderEffect {});
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
  Rect<double> target,
  ZOrder z_order
) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  if (z_order.order == 0)
    z_order.order = this->next_order++;

  WindowLayout layout { .target = target };
  SurfaceContent content {
    .geometry =
      Rect<int> {
        .width = static_cast<int>(target.width),
        .height = static_cast<int>(target.height),
      },
  };
  SceneNode node { .bounds = layout.resolve(content) };

  registry.emplace_or_replace<WindowLayout>(entity, layout);
  registry.emplace_or_replace<SurfaceContent>(entity, content);
  registry.emplace_or_replace<SceneNode>(entity, node);
  registry.emplace_or_replace<Visibility>(entity, Visibility {});
  registry.emplace_or_replace<ZOrder>(entity, z_order);
  registry.emplace_or_replace<Opacity>(entity, Opacity {});
  registry.emplace_or_replace<WindowTransition>(entity, WindowTransition {});
  registry.emplace_or_replace<WindowAnimation>(entity, WindowAnimation {});
  registry.emplace_or_replace<ShaderEffect>(entity, ShaderEffect {});
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

void UraScene::attach_session_lock(entt::entity entity, Rect<double> bounds) {
  auto& registry = this->entities.raw();
  if (!registry.valid(entity))
    return;

  registry.emplace_or_replace<WindowLayout>(
    entity,
    WindowLayout { .target = bounds }
  );
  registry.emplace_or_replace<SurfaceContent>(entity, SurfaceContent {});
  registry.emplace_or_replace<SceneNode>(
    entity,
    SceneNode { .bounds = bounds }
  );
  registry.emplace_or_replace<Visibility>(entity, Visibility {});
  registry.emplace_or_replace<ZOrder>(
    entity,
    ZOrder { UraSceneLayer::LockScreen, this->next_order++ }
  );
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

void UraScene::set_layout_target(
  entt::entity entity,
  const Rect<double>& target
) {
  auto* current = this->try_get<WindowLayout>(entity);
  if (!current || current->target == target)
    return;
  current->target = target;
  this->touch(entity, DirtyFlag::Layout);
}

void UraScene::set_surface_content(
  entt::entity entity,
  const SurfaceContent& content
) {
  auto* current = this->try_get<SurfaceContent>(entity);
  if (!current || *current == content)
    return;
  *current = content;
  this->touch(entity, DirtyFlag::Content);
}

void UraScene::set_node_bounds(
  entt::entity entity,
  const Rect<double>& bounds
) {
  auto* current = this->try_get<SceneNode>(entity);
  if (!current || current->bounds == bounds)
    return;
  auto previous = current->bounds;
  current->bounds = bounds;
  this->touch(entity, DirtyFlag::Visual, false);
  this->notify_visual_dirty(entity, previous, bounds);
}

void UraScene::sync_scene_node(entt::entity entity) {
  auto* layout = this->try_get<WindowLayout>(entity);
  auto* content = this->try_get<SurfaceContent>(entity);
  if (!layout || !content)
    return;
  this->set_node_bounds(entity, layout->resolve(*content));
}

Rect<double> UraScene::window_bounds(entt::entity entity) const {
  auto* layout = this->try_get<WindowLayout>(entity);
  auto* content = this->try_get<SurfaceContent>(entity);
  if (!layout || !content)
    return {};
  return layout->resolve(*content);
}

Rect<double> UraScene::node_bounds(entt::entity entity) const {
  auto* node = this->try_get<SceneNode>(entity);
  return node ? node->bounds : Rect<double> {};
}

WindowTransition& UraScene::ensure_window_transition(entt::entity entity) {
  if (auto* transition = this->try_get<WindowTransition>(entity))
    return *transition;
  return this->entities.raw().emplace<WindowTransition>(entity);
}

WindowAnimation& UraScene::ensure_window_animation(entt::entity entity) {
  if (auto* animation = this->try_get<WindowAnimation>(entity))
    return *animation;
  return this->entities.raw().emplace<WindowAnimation>(entity);
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
  auto* node = this->try_get<SceneNode>(entity);
  auto bounds = node ? node->bounds : Rect<double> {};
  current->value = next;
  if (!node) {
    this->touch(entity, DirtyFlag::Opacity);
    return;
  }
  // Opacity only affects the node's own rectangle. Treat it like a visual
  // change so animation ticks damage the relevant outputs instead of all
  // outputs.
  this->touch(entity, DirtyFlag::Opacity, false);
  this->notify_visual_dirty(entity, bounds, bounds);
}

void UraScene::set_effect(entt::entity entity, const Effect& effect) {
  auto* current = this->try_get<ShaderEffect>(entity);
  if (!current)
    return;
  if (current->effect == effect)
    return;
  auto* node = this->try_get<SceneNode>(entity);
  auto bounds = node ? node->bounds : Rect<double> {};
  current->effect = effect;
  this->touch(entity, DirtyFlag::Effect, false);
  if (node)
    this->notify_visual_dirty(entity, bounds, bounds);
  else
    this->notify_dirty();
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

void UraScene::set_visual_dirty_callback(
  std::function<void(entt::entity, const Rect<double>&, const Rect<double>&)>
    callback
) {
  this->visual_dirty_callback = std::move(callback);
}

const SceneSnapshot& UraScene::snapshot() const {
  if (!this->snapshot_dirty)
    return this->cached_snapshot;

  auto& result = this->cached_snapshot;
  result.items.clear();
  result.epoch = this->current_epoch;

  auto view = this->entities.raw()
                .view<
                  WindowLayout,
                  SurfaceContent,
                  SceneNode,
                  Visibility,
                  ZOrder,
                  Opacity>();
  for (auto [entity, layout, content, node, visibility, z_order, opacity] :
       view.each()) {
    SceneRole role = SceneRole::Primitive;
    if (this->entities.raw().all_of<ToplevelRef>(entity))
      role = SceneRole::Toplevel;
    else if (this->entities.raw().all_of<LayerShellRef>(entity))
      role = SceneRole::LayerShell;
    else if (this->entities.raw().all_of<PopupRef>(entity))
      role = SceneRole::Popup;
    else if (this->entities.raw().all_of<SessionLockRef>(entity))
      role = SceneRole::SessionLock;

    auto* target = this->entities.raw().try_get<RenderTarget>(entity);
    auto* shader_effect = this->entities.raw().try_get<ShaderEffect>(entity);
    result.items.push_back(
      SceneItemSnapshot {
        .id = UraEntityRegistry::to_id(entity),
        .role = role,
        .layout = layout,
        .content = content,
        .node = node,
        .visibility = visibility,
        .z_order = z_order,
        .opacity = opacity,
        .effect = shader_effect ? shader_effect->effect : Effect {},
        .render_target = target ? target->resource : 0,
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
    auto& bounds = item->node.bounds;
    auto right = bounds.x + bounds.width;
    auto bottom = bounds.y + bounds.height;
    if (x >= bounds.x && x < right && y >= bounds.y && y < bottom)
      return item->id;
  }
  return {};
}

void UraScene::touch(entt::entity entity, DirtyFlag flag, bool notify) {
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
  if (notify)
    this->notify_dirty();
  else
    this->snapshot_dirty = true;
}

void UraScene::destroy(entt::entity entity) {
  this->entities.destroy(entity);
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

void UraScene::notify_visual_dirty(
  entt::entity entity,
  const Rect<double>& previous,
  const Rect<double>& current
) {
  this->snapshot_dirty = true;
  if (this->visual_dirty_callback)
    this->visual_dirty_callback(entity, previous, current);
  else if (this->dirty_callback)
    this->dirty_callback();
}

entt::registry& UraScene::raw() {
  return this->entities.raw();
}

const entt::registry& UraScene::raw() const {
  return this->entities.raw();
}

} // namespace ura
