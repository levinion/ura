#include "ura/scene/frame_planner.hpp"

#include <iostream>

namespace {

ura::UraOutput* fake_output(uintptr_t value) {
  return reinterpret_cast<ura::UraOutput*>(value);
}

ura::FrameSurfaceSnapshot
frozen_at(uint64_t content_id, int x, int y, int width, int height) {
  return ura::FrameSurfaceSnapshot {
    .surface = nullptr,
    .texture = reinterpret_cast<wlr_texture*>(content_id + 0x1000),
    .dst_box = { x, y, width, height },
    .seq = 0,
    .output_name = {},
    .content_id = content_id,
    .effect = {},
  };
}

ura::FrameSurfaceSnapshot surface_at(
  uintptr_t surface,
  int x,
  int y,
  int width,
  int height,
  uint32_t seq
) {
  return ura::FrameSurfaceSnapshot {
    .surface = reinterpret_cast<wlr_surface*>(surface),
    .texture = reinterpret_cast<wlr_texture*>(surface + 0x1000),
    .dst_box = { x, y, width, height },
    .seq = seq,
    .output_name = {},
    .effect = {},
  };
}

bool test_damage_and_commit_state() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x1000);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 100, 100 },
  };

  ura::FrameWorldSnapshot world;
  world.surfaces.push_back(surface_at(0x2000, 10, 10, 40, 40, 1));
  world.operations.push_back({ ura::FrameOperationKind::Surface, 0 });

  ura::UraFramePlanner::OutputState next;
  auto first = planner.build(output_state, *state, next, world);
  if (
    !first.full_redraw || first.surfaces.size() != 1 || first.damage.size() != 1
  )
    return false;
  planner.commit_state(output, std::move(next));

  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState next_idle;
  auto idle = planner.build(output_state, *state, next_idle, world);
  if (idle.full_redraw || !idle.damage.empty() || idle.surfaces.size() != 1)
    return false;
  planner.commit_state(output, std::move(next_idle));

  world.surfaces[0].seq = 2;
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState next_commit;
  auto committed = planner.build(output_state, *state, next_commit, world);
  if (committed.full_redraw || committed.damage.size() != 1)
    return false;
  planner.commit_state(output, std::move(next_commit));

  world.surfaces.push_back(surface_at(0x3000, 200, 200, 40, 40, 1));
  world.operations.push_back({ ura::FrameOperationKind::Surface, 1 });
  world.surfaces.push_back(surface_at(0x4000, 10, 10, 10, 10, 1));
  world.surfaces.back().output_name = "OTHER";
  world.operations.push_back({ ura::FrameOperationKind::Surface, 2 });

  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState next_filter;
  auto filtered = planner.build(output_state, *state, next_filter, world);
  return filtered.surfaces.size() == 1
    && filtered.surfaces[0].surface == world.surfaces[0].surface;
}

bool test_frozen_surface_identity() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x12000);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 200, 200 },
  };

  ura::FrameWorldSnapshot world;
  world.surfaces.push_back(frozen_at(1, 10, 10, 40, 40));
  world.surfaces.push_back(frozen_at(2, 80, 10, 40, 40));
  world.operations.push_back({ ura::FrameOperationKind::Surface, 0 });
  world.operations.push_back({ ura::FrameOperationKind::Surface, 1 });

  ura::UraFramePlanner::OutputState next;
  auto first = planner.build(output_state, *state, next, world);
  if (!first.full_redraw || first.surfaces.size() != 2)
    return false;
  planner.commit_state(output, std::move(next));

  world.surfaces[1].dst_box.x = 100;
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState moved_next;
  auto moved = planner.build(output_state, *state, moved_next, world);
  if (moved.full_redraw || moved.damage.empty() || moved.surfaces.size() != 2)
    return false;
  planner.commit_state(output, std::move(moved_next));

  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState idle_next;
  auto idle = planner.build(output_state, *state, idle_next, world);
  if (idle.full_redraw || !idle.damage.empty() || idle.surfaces.size() != 2)
    return false;
  planner.commit_state(output, std::move(idle_next));

  // Changing an effect invalidates the surface's damage even when its
  // geometry and texture sequence are unchanged.
  auto pipeline = std::make_shared<ura::EffectPipeline>();
  pipeline->handle = ura::EffectHandle { .id = 200, .generation = 1 };
  pipeline->max_outset = { .left = 4, .top = 4, .right = 4, .bottom = 4 };
  world.surfaces[0].effect = ura::EffectInstance {
    .pipeline = pipeline,
    .version = 1,
    .outset = pipeline->max_outset,
  };
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState effect_next;
  auto effect = planner.build(output_state, *state, effect_next, world);
  if (
    effect.full_redraw || effect.damage.empty() || effect.surfaces.size() != 2
  )
    return false;
  planner.commit_state(output, std::move(effect_next));

  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState effect_idle_next;
  auto effect_idle =
    planner.build(output_state, *state, effect_idle_next, world);
  return !effect_idle.full_redraw && effect_idle.damage.empty()
    && effect_idle.surfaces.size() == 2;
}

bool test_locked_world_filters_scene_surfaces() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x15000);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 100, 100 },
  };

  ura::FrameWorldSnapshot world;
  world.locked = true;
  world.surfaces.push_back(surface_at(0x16000, 0, 0, 100, 100, 1));
  world.surfaces.push_back(surface_at(0x17000, 0, 0, 100, 100, 1));
  world.surfaces.back().lock_surface = true;
  world.operations.push_back({ ura::FrameOperationKind::Surface, 0 });
  world.operations.push_back({ ura::FrameOperationKind::Surface, 1 });

  ura::UraFramePlanner::OutputState next;
  auto plan = planner.build(output_state, *state, next, world);
  return plan.full_redraw && plan.surfaces.size() == 1
    && plan.surfaces[0].surface == world.surfaces[1].surface
    && plan.background_color == std::array<float, 4> { { 0.f, 0.f, 0.f, 1.f } };
}

bool test_rect_damage() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x5000);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 100, 100 },
  };

  ura::FrameWorldSnapshot world;
  world.rects.push_back(
    ura::FrameRectSnapshot {
      .rect = {
        .dst_box = { 4, 4, 10, 10 },
        .color = { 1.f, 0.f, 0.f, 1.f },
        .owner = 1,
        .index = 0,
        .effect = {},
      },
      .output_name = {},
    }
  );
  world.operations.push_back({ ura::FrameOperationKind::Rect, 0 });

  ura::UraFramePlanner::OutputState next;
  auto first = planner.build(output_state, *state, next, world);
  if (!first.full_redraw || first.rects.size() != 1)
    return false;
  planner.commit_state(output, std::move(next));

  world.rects[0].rect.color = { 0.f, 0.f, 1.f, 1.f };
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState next_color;
  auto color = planner.build(output_state, *state, next_color, world);
  return !color.full_redraw && !color.damage.empty();
}

bool test_effect_outset_damage() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x5200);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 200, 200 },
  };

  auto pipeline = std::make_shared<ura::EffectPipeline>();
  pipeline->handle = ura::EffectHandle { .id = 7, .generation = 1 };
  pipeline->max_outset = { .left = 5, .top = 5, .right = 5, .bottom = 5 };

  ura::EffectInstance instance;
  instance.pipeline = pipeline;
  instance.version = 1;
  instance.outset = { .left = 10, .top = 0, .right = 5, .bottom = 0 };

  ura::FrameWorldSnapshot world;
  world.surfaces.push_back(frozen_at(1, 50, 50, 20, 20));
  world.operations.push_back({ ura::FrameOperationKind::Surface, 0 });

  ura::UraFramePlanner::OutputState next;
  auto first = planner.build(output_state, *state, next, world);
  if (!first.full_redraw)
    return false;
  planner.commit_state(output, std::move(next));

  world.surfaces[0].effect = instance;
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState effect_next;
  auto effect = planner.build(output_state, *state, effect_next, world);
  auto expanded = wlr_box { .x = 40, .y = 50, .width = 35, .height = 20 };
  auto found_expanded =
    std::ranges::any_of(effect.damage, [&](const wlr_box& box) {
      return box.x == expanded.x && box.y == expanded.y
        && box.width == expanded.width && box.height == expanded.height;
    });
  if (effect.full_redraw || !found_expanded)
    return false;
  planner.commit_state(output, std::move(effect_next));

  // A uniform/version change must invalidate the expanded bounds even when the
  // surface rectangle and sequence are unchanged.
  world.surfaces[0].effect.version = 2;
  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState uniform_next;
  auto uniform = planner.build(output_state, *state, uniform_next, world);
  auto expanded_count =
    std::ranges::count_if(uniform.damage, [&](const wlr_box& box) {
      return box.x == expanded.x && box.y == expanded.y
        && box.width == expanded.width && box.height == expanded.height;
    });
  return !uniform.full_redraw && expanded_count == 2;
}

bool test_abort_frame_retry() {
  ura::UraFramePlanner planner;
  auto* output = fake_output(0x9000);
  auto* state = planner.prepare_output(output);

  ura::OutputFrameState output_state {
    .output = output,
    .enabled = true,
    .name = "OUT-1",
    .logical_box = { 0, 0, 100, 100 },
  };

  ura::FrameWorldSnapshot world;
  world.surfaces.push_back(surface_at(0xA000, 10, 10, 40, 40, 1));
  world.operations.push_back({ ura::FrameOperationKind::Surface, 0 });

  ura::UraFramePlanner::OutputState next;
  auto first = planner.build(output_state, *state, next, world);
  if (!first.full_redraw)
    return false;
  planner.commit_state(output, std::move(next));

  auto* associated = reinterpret_cast<wlr_surface*>(0xB000);
  state = planner.prepare_output(output);
  state->entered_surfaces.insert(associated);
  state->notified_scales[associated] = 1;

  // A frame was planned but its render failed, so its candidate state must not
  // be committed. abort_frame() keeps the protocol association but asks for a
  // full repaint on the retry.
  ura::UraFramePlanner::OutputState failed_next;
  auto failed = planner.build(output_state, *state, failed_next, world);
  if (failed.full_redraw || !failed.damage.empty())
    return false;
  planner.abort_frame(output);

  state = planner.prepare_output(output);
  if (!state->entered_surfaces.contains(associated))
    return false;
  if (!state->notified_scales.contains(associated))
    return false;

  ura::UraFramePlanner::OutputState retry_next;
  auto retry = planner.build(output_state, *state, retry_next, world);
  if (!retry.full_redraw || retry.damage.size() != 1)
    return false;
  planner.commit_state(output, std::move(retry_next));

  state = planner.prepare_output(output);
  ura::UraFramePlanner::OutputState idle_next;
  auto idle = planner.build(output_state, *state, idle_next, world);
  return !idle.full_redraw && idle.damage.empty() && idle.surfaces.size() == 1;
}

} // namespace

int main() {
  if (!test_damage_and_commit_state()) {
    std::cerr << "damage and commit state test failed\n";
    return 1;
  }
  if (!test_frozen_surface_identity()) {
    std::cerr << "frozen surface identity test failed\n";
    return 1;
  }
  if (!test_locked_world_filters_scene_surfaces()) {
    std::cerr << "locked world filter test failed\n";
    return 1;
  }
  if (!test_rect_damage()) {
    std::cerr << "rect damage test failed\n";
    return 1;
  }
  if (!test_effect_outset_damage()) {
    std::cerr << "effect outset damage test failed\n";
    return 1;
  }
  if (!test_abort_frame_retry()) {
    std::cerr << "abort frame retry test failed\n";
    return 1;
  }
  return 0;
}
