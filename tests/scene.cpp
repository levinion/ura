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

ura::UraSessionLockSurface* fake_session_lock(uintptr_t value) {
  return reinterpret_cast<ura::UraSessionLockSurface*>(value);
}

bool test_snapshot_order() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);

  auto first = entities.create_toplevel(fake_toplevel(0x1000));
  auto second = entities.create_toplevel(fake_toplevel(0x2000));

  scene.attach_toplevel(
    first,
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_toplevel(
    second,
    ura::Rect<double> { .x = 10, .y = 10, .width = 50, .height = 50 },
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
    ura::Rect<double> { .width = 320, .height = 200 },
    ura::WindowTags {},
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  auto initial_epoch = scene.epoch();
  scene.set_visible(entity, true);
  auto visible_epoch = scene.epoch();
  scene.set_layout_target(
    entity,
    ura::Rect<double> { .width = 400, .height = 300 }
  );

  auto snapshot = scene.snapshot();
  auto updated = snapshot.items.size() == 1
    && snapshot.items[0].layout.target.width == 400
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
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_layer_shell(
    layer_shell,
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 50 },
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
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::WindowTags { { "1" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.attach_toplevel(
    second,
    ura::Rect<double> { .x = 10, .y = 10, .width = 50, .height = 50 },
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

  scene.set_layout_target(
    first,
    ura::Rect<double> { .x = 20, .y = 30, .width = 100, .height = 100 }
  );
  auto* updated = &scene.snapshot();
  if (updated != cached || updated->items[0].layout.target.x != 20)
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
    ura::Rect<double> { .width = 10, .height = 10 },
    ura::WindowTags { { "alpha", "beta" } },
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );

  auto& dirty = scene.get<ura::DirtyState>(entity);
  dirty.flags = 0;
  scene.set_opacity(entity, 1.5f);
  scene.set_tags(entity, ura::WindowTags { { "gamma" } });
  scene.set_layout_target(
    entity,
    ura::Rect<double> { .width = 20, .height = 30 }
  );
  scene.set_layer(entity, ura::UraSceneLayer::Top);
  auto pipeline = std::make_shared<ura::EffectPipeline>();
  pipeline->handle = ura::EffectHandle { .id = 300, .generation = 1 };
  pipeline->max_outset = { .left = 4, .top = 4, .right = 4, .bottom = 4 };
  scene.set_effect(
    entity,
    ura::EffectInstance {
      .pipeline = pipeline,
      .version = 1,
      .outset = pipeline->max_outset,
    }
  );

  auto& opacity = scene.get<ura::Opacity>(entity);
  auto& tags = scene.get<ura::WindowTags>(entity);
  auto& z_order = scene.get<ura::ZOrder>(entity);

  auto layout_flag =
    dirty.flags & static_cast<uint32_t>(ura::DirtyFlag::Layout);
  auto effect_flag =
    dirty.flags & static_cast<uint32_t>(ura::DirtyFlag::Effect);
  auto snapshot = scene.snapshot();
  return opacity.value == 1.f && tags.contains("gamma")
    && !tags.contains("alpha") && z_order.layer == ura::UraSceneLayer::Top
    && dirty.epoch > 0 && layout_flag != 0 && effect_flag != 0
    && snapshot.items.size() == 1 && snapshot.items[0].effect.active()
    && snapshot.items[0].effect.pipeline == pipeline;
}

bool test_rotation_hit_test() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);
  auto entity = entities.create_toplevel(fake_toplevel(0x9500));

  scene.attach_toplevel(
    entity,
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::WindowTags {},
    ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
  );
  scene.set_visible(entity, true);
  scene.set_node_rotation(entity, 0.78539816339f);

  auto center = scene.topmost_at(50, 50);
  auto inside = scene.topmost_at(50, 10);
  auto rotated_outside = scene.topmost_at(5, 5);
  return center && *center == ura::UraEntityRegistry::to_id(entity) && inside
    && *inside == ura::UraEntityRegistry::to_id(entity) && !rotated_outside;
}

bool test_rect_intersection() {
  ura::Rect<double> area { .x = 0, .y = 0, .width = 10, .height = 10 };
  auto overlap = area.intersection(
    ura::Rect<double> { .x = 5, .y = 5, .width = 10, .height = 10 }
  );
  if (overlap != ura::Rect<double> { .x = 5, .y = 5, .width = 5, .height = 5 })
    return false;
  if (!area.intersects(
        ura::Rect<double> { .x = 5, .y = 5, .width = 1, .height = 1 }
      ))
    return false;
  if (
    area.intersects(
      ura::Rect<double> { .x = 10, .y = 0, .width = 5, .height = 5 }
    )
  )
    return false;
  if (
    area.intersects(
      ura::Rect<double> { .x = 20, .y = 20, .width = 5, .height = 5 }
    )
  )
    return false;
  if (ura::Rect<double> {}.intersects(area))
    return false;

  // Cross-scalar queries are used by output scheduling (double visual bounds
  // against integer output geometry).
  auto cross = area.intersection(
    ura::Rect<int> { .x = 5, .y = 5, .width = 20, .height = 20 }
  );
  if (cross != ura::Rect<double> { .x = 5, .y = 5, .width = 5, .height = 5 })
    return false;

  wlr_box box { .x = 1, .y = 2, .width = 3, .height = 4 };
  auto converted = ura::Rect<double>::from(box);
  auto round_trip = converted.to_wlr_box();
  return round_trip.x == box.x && round_trip.y == box.y
    && round_trip.width == box.width && round_trip.height == box.height;
}

bool test_session_lock_role() {
  ura::UraEntityRegistry entities;
  ura::UraScene scene(entities);
  auto entity = entities.create_session_lock(fake_session_lock(0x9000));

  scene.attach_session_lock(
    entity,
    ura::Rect<double> { .x = 0, .y = 0, .width = 640, .height = 480 }
  );
  scene.set_visible(entity, true);

  auto snapshot = scene.snapshot();
  if (snapshot.items.size() != 1)
    return false;
  auto& item = snapshot.items[0];
  if (
    item.role != ura::SceneRole::SessionLock
    || item.z_order.layer != ura::UraSceneLayer::LockScreen
    || item.node.bounds.width != 640 || item.node.bounds.height != 480
  )
    return false;

  auto hit = scene.topmost_at(10, 10);
  if (!hit || *hit != ura::UraEntityRegistry::to_id(entity))
    return false;

  entities.destroy(entity);
  auto empty = scene.snapshot();
  return empty.items.empty();
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
  else if (name == "session-lock")
    passed = test_session_lock_role();
  else if (name == "rect")
    passed = test_rect_intersection();
  else if (name == "rotation")
    passed = test_rotation_hit_test();
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
