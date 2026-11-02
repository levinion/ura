#include "ura/core/entity.hpp"
#include "ura/scene/layer.hpp"
#include "ura/scene/scene.hpp"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

ura::UraToplevel* fake_toplevel(uintptr_t value) {
  return reinterpret_cast<ura::UraToplevel*>(value);
}

ura::UraLayerShell* fake_layer_shell(uintptr_t value) {
  return reinterpret_cast<ura::UraLayerShell*>(value);
}

bool test_snapshot_order() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);

  auto first = entities.create_toplevel(fake_toplevel(0x1000));
  auto second = entities.create_toplevel(fake_toplevel(0x2000));

  scene.attach_toplevel(
    first,
    ura::Transform { 0, 0, 0.f, 1.0 },
    ura::Geometry { 100, 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_toplevel(
    second,
    ura::Transform { 10, 10, 0.f, 1.0 },
    ura::Geometry { 50, 50 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Floating, 0 }
  );
  scene.set_visible(first, true);
  scene.set_visible(second, true);

  auto snapshot = scene.snapshot();
  if (snapshot.items.size() != 2)
    return false;
  if (snapshot.items[0].id != ura::UraEntityRegistry::to_id(first))
    return false;
  if (snapshot.items[1].id != ura::UraEntityRegistry::to_id(second))
    return false;
  if (snapshot.items[0].role != ura::SceneRole::Toplevel)
    return false;

  // The floating window is above the normal window and wins the rectangle
  // query where they overlap.
  auto overlapping = scene.topmost_at(20, 20);
  auto normal_only = scene.topmost_at(90, 90);
  return overlapping && *overlapping == ura::UraEntityRegistry::to_id(second)
    && normal_only && *normal_only == ura::UraEntityRegistry::to_id(first);
}

bool test_epoch_and_stale_handle() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);
  auto entity = entities.create_toplevel(fake_toplevel(0x3000));

  scene.attach_toplevel(
    entity,
    ura::Transform {},
    ura::Geometry { 320, 200 },
    ura::WindowTags {},
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  auto initial_epoch = scene.epoch();
  scene.set_visible(entity, true);
  auto visible_epoch = scene.epoch();
  scene.set_geometry(entity, ura::Geometry { 400, 300 });

  auto snapshot = scene.snapshot();
  auto updated = snapshot.items.size() == 1
    && snapshot.items[0].geometry.width == 400
    && snapshot.items[0].visibility.visible && scene.epoch() > visible_epoch
    && visible_epoch > initial_epoch;

  entities.destroy(entity);
  auto after_destroy = scene.snapshot();
  return updated && after_destroy.items.empty() && !scene.topmost_at(0, 0);
}

bool test_layer_shell_order() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);

  auto toplevel = entities.create_toplevel(fake_toplevel(0x5000));
  auto layer_shell = entities.create_layer_shell(fake_layer_shell(0x6000));

  scene.attach_toplevel(
    toplevel,
    ura::Transform { 0, 0, 0.f, 1.0 },
    ura::Geometry { 100, 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_layer_shell(
    layer_shell,
    ura::Transform { 0, 0, 0.f, 1.0 },
    ura::Geometry { 100, 50 },
    ura::ZOrder { ura::UraSceneLayer::Top, 0 }
  );
  scene.set_visible(toplevel, true);
  scene.set_visible(layer_shell, true);

  auto snapshot = scene.snapshot();
  if (snapshot.items.size() != 2)
    return false;
  if (snapshot.items[0].role != ura::SceneRole::Toplevel)
    return false;
  if (snapshot.items[1].role != ura::SceneRole::LayerShell)
    return false;

  auto layer_hit = scene.topmost_at(25, 25);
  auto toplevel_hit = scene.topmost_at(25, 75);
  return layer_hit && *layer_hit == ura::UraEntityRegistry::to_id(layer_shell)
    && toplevel_hit && *toplevel_hit == ura::UraEntityRegistry::to_id(toplevel);
}

bool test_snapshot_cache() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);
  auto first = entities.create_toplevel(fake_toplevel(0x7000));
  auto second = entities.create_toplevel(fake_toplevel(0x8000));

  scene.attach_toplevel(
    first,
    ura::Transform { 0, 0, 0.f, 1.0 },
    ura::Geometry { 100, 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_toplevel(
    second,
    ura::Transform { 10, 10, 0.f, 1.0 },
    ura::Geometry { 50, 50 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Floating, 0 }
  );
  scene.set_visible(first, true);
  scene.set_visible(second, true);

  auto* cached = &scene.snapshot();
  if (cached->items.size() != 2)
    return false;
  if (&scene.snapshot() != cached)
    return false;

  scene.set_transform(first, ura::Transform { 20, 30, 0.f, 1.0 });
  auto* updated = &scene.snapshot();
  if (updated != cached || updated->items[0].transform.x != 20)
    return false;

  auto epoch_before_destroy = scene.epoch();
  entities.destroy(first);
  auto epoch_after_destroy = scene.epoch();
  auto* after_destroy = &scene.snapshot();
  return epoch_after_destroy > epoch_before_destroy && after_destroy == cached
    && after_destroy->items.size() == 1
    && after_destroy->items[0].id == ura::UraEntityRegistry::to_id(second);
}

bool test_components_and_dirty_state() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);
  auto entity = entities.create_toplevel(fake_toplevel(0x4000));

  scene.attach_toplevel(
    entity,
    ura::Transform {},
    ura::Geometry { 10, 10 },
    ura::WindowTags { { "alpha", "beta" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );

  auto& dirty = scene.get<ura::DirtyState>(entity);
  dirty.flags = 0;
  scene.set_opacity(entity, 1.5f);
  scene.set_tags(entity, ura::WindowTags { { "gamma" } });
  scene.set_geometry(entity, ura::Geometry { 20, 30 });
  scene.set_layer(entity, ura::UraSceneLayer::Top);

  auto& opacity = scene.get<ura::Opacity>(entity);
  auto& tags = scene.get<ura::WindowTags>(entity);
  auto& z_order = scene.get<ura::ZOrder>(entity);

  auto geometry_flag =
    dirty.flags & static_cast<uint32_t>(ura::DirtyFlag::Geometry);
  return opacity.value == 1.f && tags.contains("gamma")
    && !tags.contains("alpha") && z_order.layer == ura::UraSceneLayer::Top
    && dirty.epoch > 0 && geometry_flag != 0;
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test case name\n";
    return 2;
  }

  const std::string_view name { argv[1] };
  bool passed = false;
  if (name == "snapshot-order")
    passed = test_snapshot_order();
  else if (name == "stale-handle")
    passed = test_epoch_and_stale_handle();
  else if (name == "components")
    passed = test_components_and_dirty_state();
  else if (name == "layer-shell")
    passed = test_layer_shell_order();
  else if (name == "snapshot-cache")
    passed = test_snapshot_cache();
  else {
    std::cerr << "Unknown test case: " << name << '\n';
    return 2;
  }

  if (!passed) {
    std::cerr << "Test failed: " << name << '\n';
    return 1;
  }
  return 0;
}
