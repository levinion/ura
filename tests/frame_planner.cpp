#include "ura/scene/frame_planner.hpp"

#include <iostream>

namespace {

ura::UraOutput* fake_output(uintptr_t value) {
  return reinterpret_cast<ura::UraOutput*>(value);
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

} // namespace

int main() {
  if (!test_damage_and_commit_state()) {
    std::cerr << "damage and commit state test failed\n";
    return 1;
  }
  if (!test_rect_damage()) {
    std::cerr << "rect damage test failed\n";
    return 1;
  }
  return 0;
}
