#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

#include "ura/core/entity.hpp"
#include "ura/scene/animation.hpp"
#include "ura/scene/animation_system.hpp"
#include "ura/scene/layer.hpp"
#include "ura/scene/scene.hpp"

namespace {

ura::UraToplevel* fake_toplevel(uintptr_t value) {
  return reinterpret_cast<ura::UraToplevel*>(value);
}

constexpr auto epsilon = 1e-6;

bool near(double lhs, double rhs) {
  return std::abs(lhs - rhs) < epsilon;
}

bool near(const ura::Rect<double>& lhs, const ura::Rect<double>& rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.width, rhs.width)
    && near(lhs.height, rhs.height);
}

struct Fixture {
  ura::UraEntityRegistry entities;
  ura::UraScene scene { entities };
  ura::UraAnimationSystem animations { scene };
  entt::entity entity { entt::null };

  Fixture() {
    this->entity = this->entities.create_toplevel(fake_toplevel(0x1000));
    this->scene.attach_toplevel(
      this->entity,
      ura::Rect<double> { .x = 10, .y = 20, .width = 100, .height = 100 },
      ura::WindowTags { { "1" } },
      ura::ZOrder { ura::UraSceneLayer::Normal, 0 }
    );
    this->scene.set_visible(this->entity, true);
  }
};

bool test_easing_curve() {
  auto linear = ura::EasingCurve::from_name("linear");
  auto cubic = ura::EasingCurve::from_name("in_out_cubic");
  auto unknown = ura::EasingCurve::from_name("does-not-exist");

  if (!near(linear.sample(0.0), 0.0) || !near(linear.sample(1.0), 1.0))
    return false;
  if (!near(linear.sample(0.25), 0.25))
    return false;
  if (!near(cubic.sample(0.0), 0.0) || !near(cubic.sample(1.0), 1.0))
    return false;
  // The default fallback is in-out cubic.
  if (!near(cubic.sample(0.5), unknown.sample(0.5)))
    return false;

  auto custom = ura::EasingCurve::from_samples({ 0.f, 0.25f, 1.f });
  if (!near(custom.sample(0.0), 0.0) || !near(custom.sample(1.0), 1.0))
    return false;
  if (std::abs(custom.sample(0.5) - 0.25f) > 0.01f)
    return false;

  return cubic.sample(0.5) > 0.49f && cubic.sample(0.5) < 0.51f;
}

bool test_layout_content_and_node_are_separate() {
  Fixture fixture;
  auto* layout = fixture.scene.try_get<ura::WindowLayout>(fixture.entity);
  auto* content = fixture.scene.try_get<ura::SurfaceContent>(fixture.entity);
  auto* node = fixture.scene.try_get<ura::SceneNode>(fixture.entity);
  if (!layout || !content || !node || content->geometry.width != 100)
    return false;

  // Changing the layout target must not move the visible SceneNode.
  auto before = fixture.scene.node_bounds(fixture.entity);
  fixture.scene.set_layout_target(
    fixture.entity,
    ura::Rect<double> { .x = 10, .y = 20, .width = 200, .height = 50 }
  );
  auto after = fixture.scene.node_bounds(fixture.entity);
  if (!near(before, after) || after.width != 100 || after.height != 100)
    return false;

  // A committed content size does not move SceneNode by itself either. The
  // transition/animation layer decides when the new content becomes visible.
  fixture.scene.set_surface_content(
    fixture.entity,
    ura::SurfaceContent { .geometry = { .width = 200, .height = 100 } }
  );
  auto content_changed = fixture.scene.node_bounds(fixture.entity);
  if (!near(content_changed, after))
    return false;

  // SceneNode.bounds is the authority for hit testing, even when the client
  // content still has a different size.
  auto visual_old = ura::Rect<double> {};
  auto visual_new = ura::Rect<double> {};
  auto visual_updates = 0;
  fixture.scene.set_visual_dirty_callback([&](
                                            entt::entity,
                                            const ura::Rect<double>& previous,
                                            const ura::Rect<double>& current
                                          ) {
    visual_old = previous;
    visual_new = current;
    visual_updates++;
  });
  fixture.scene.set_node_bounds(
    fixture.entity,
    ura::Rect<double> { .x = 40.0, .y = 50.0, .width = 200.0, .height = 150.0 }
  );
  if (
    visual_updates != 1 || !near(visual_old, after)
    || !near(visual_new, { 40.0, 50.0, 200.0, 150.0 })
  )
    return false;
  auto bounds = fixture.scene.node_bounds(fixture.entity);
  auto hit_inside = fixture.scene.topmost_at(45, 55);
  auto hit_outside = fixture.scene.topmost_at(39, 55);
  return near(bounds, { 40.0, 50.0, 200.0, 150.0 }) && hit_inside
    && *hit_inside == ura::UraEntityRegistry::to_id(fixture.entity)
    && !hit_outside;
}

bool test_flip_interpolation_and_finish() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  auto options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
  };

  // Simulate a committed client size change. The animation still starts from
  // the old visible rectangle, so there is no jump when the new texture lands.
  fixture.scene.set_surface_content(
    fixture.entity,
    ura::SurfaceContent { .geometry = { .width = 200, .height = 100 } }
  );
  fixture.animations.animate_from(
    fixture.entity,
    ura::Rect<double> { .x = 10, .y = 20, .width = 100, .height = 100 },
    ura::Rect<double> { .x = 50, .y = 70, .width = 200, .height = 100 },
    options,
    start
  );

  if (!fixture.animations.running(fixture.entity))
    return false;
  if (!near(fixture.scene.node_bounds(fixture.entity), { 10, 20, 100, 100 }))
    return false;

  fixture.animations.tick(start + std::chrono::milliseconds(50));
  auto halfway = fixture.scene.node_bounds(fixture.entity);
  if (!near(halfway, { 30, 45, 150, 100 }))
    return false;

  fixture.animations.tick(start + std::chrono::milliseconds(100));
  auto finished = fixture.scene.node_bounds(fixture.entity);
  return near(finished, { 50, 70, 200, 100 })
    && !fixture.animations.running(fixture.entity);
}

bool test_retarget_from_current_visual() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  auto options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
  };

  fixture.animations.animate_from(
    fixture.entity,
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::Rect<double> { .x = 100, .y = 0, .width = 100, .height = 100 },
    options,
    start
  );
  fixture.animations.tick(start + std::chrono::milliseconds(50));
  auto current = fixture.scene.node_bounds(fixture.entity);
  if (!near(current.x, 50.0))
    return false;

  auto retarget = start + std::chrono::milliseconds(50);
  fixture.animations.animate(
    fixture.entity,
    ura::Rect<double> { .x = 200, .y = 0, .width = 100, .height = 100 },
    options,
    retarget
  );
  fixture.animations.tick(retarget + std::chrono::milliseconds(50));
  auto halfway = fixture.scene.node_bounds(fixture.entity);
  // 50 + (200 - 50) / 2, not a jump to the old target (100).
  return near(halfway, { 125, 0, 100, 100 });
}

bool test_resize_timeout_uses_committed_size() {
  Fixture fixture;
  auto now = ura::AnimationClock::now();
  fixture.scene.set_layout_target(
    fixture.entity,
    ura::Rect<double> { .x = 200, .y = 100, .width = 400, .height = 300 }
  );

  auto& transition = fixture.scene.ensure_window_transition(fixture.entity);
  transition.phase = ura::WindowTransitionPhase::WaitingForCommit;
  transition.from = { 10, 20, 100, 100 };
  transition.requested_target = { 200, 100, 400, 300 };
  transition.generation = 1;
  transition.deadline = now - std::chrono::milliseconds(1);
  transition.options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
  };

  fixture.animations.tick(now);
  if (transition.phase != ura::WindowTransitionPhase::Animating)
    return false;
  // The client never committed the requested size, so the fallback animates
  // to the target anchor using the last valid content size (100x100).
  if (!near(transition.resolved_target, { 200, 100, 100, 100 }))
    return false;

  fixture.animations.tick(now + std::chrono::milliseconds(100));
  auto finished = fixture.scene.node_bounds(fixture.entity);
  return transition.phase == ura::WindowTransitionPhase::Idle
    && near(finished, { 200, 100, 100, 100 })
    && !fixture.animations.running(fixture.entity);
}

bool test_opacity_only_animation() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  auto bounds = fixture.scene.node_bounds(fixture.entity);
  fixture.scene.set_opacity(fixture.entity, 0.f);

  auto options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
    .animate_opacity = true,
    .opacity = 1.f,
  };
  fixture.animations
    .animate_from(fixture.entity, bounds, bounds, options, start);
  if (!fixture.animations.running(fixture.entity))
    return false;

  fixture.animations.tick(start + std::chrono::milliseconds(50));
  auto halfway = fixture.scene.get<ura::Opacity>(fixture.entity).value;
  fixture.animations.tick(start + std::chrono::milliseconds(100));
  auto finished = fixture.scene.get<ura::Opacity>(fixture.entity).value;
  return near(halfway, 0.5) && near(finished, 1.0)
    && !fixture.animations.running(fixture.entity);
}

bool test_opacity_survives_retarget() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  auto bounds = fixture.scene.node_bounds(fixture.entity);
  fixture.scene.set_opacity(fixture.entity, 0.f);

  auto open_options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
    .animate_opacity = true,
    .opacity = 1.f,
  };
  fixture.animations
    .animate_from(fixture.entity, bounds, bounds, open_options, start);
  fixture.animations.tick(start + std::chrono::milliseconds(50));

  // A layout move interrupts the open animation. The bounds retarget must not
  // cancel the independent opacity target.
  auto move_options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
  };
  auto retarget = start + std::chrono::milliseconds(50);
  fixture.animations.animate(
    fixture.entity,
    ura::Rect<double> { .x = 30, .y = 0, .width = 100, .height = 100 },
    move_options,
    retarget
  );
  fixture.animations.tick(retarget + std::chrono::milliseconds(50));
  auto halfway = fixture.scene.get<ura::Opacity>(fixture.entity).value;
  fixture.animations.tick(retarget + std::chrono::milliseconds(100));
  auto finished = fixture.scene.get<ura::Opacity>(fixture.entity).value;
  return near(halfway, 0.75) && near(finished, 1.0);
}

bool test_rotation_animation() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  auto bounds = fixture.scene.node_bounds(fixture.entity);
  auto options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(100),
    .easing = ura::EasingCurve::from_name("linear"),
    .animate_rotation = true,
    .rotation = 1.57079632679f,
  };
  fixture.animations
    .animate_from(fixture.entity, bounds, bounds, options, start);
  if (!fixture.animations.running(fixture.entity))
    return false;

  fixture.animations.tick(start + std::chrono::milliseconds(50));
  auto halfway = fixture.scene.get<ura::SceneNode>(fixture.entity).rotation;
  fixture.animations.tick(start + std::chrono::milliseconds(100));
  auto finished = fixture.scene.get<ura::SceneNode>(fixture.entity).rotation;
  return near(halfway, 0.78539816339) && near(finished, 1.57079632679);
}

bool test_effect_uniform_animation() {
  Fixture fixture;
  auto pipeline = std::make_shared<ura::EffectPipeline>();
  pipeline->handle = ura::EffectHandle { .id = 77, .generation = 1 };
  pipeline->name = "animation-test";
  pipeline->uniforms = {
    ura::EffectUniformDef {
      .name = "amount",
      .type = ura::EffectUniformType::Float,
    },
  };

  ura::EffectInstance instance;
  instance.pipeline = pipeline;
  instance.uniforms = {
    ura::EffectUniform {
      .name = "amount",
      .type = ura::EffectUniformType::Float,
      .values = { { 0.f, 0.f, 0.f, 0.f } },
    },
  };
  fixture.scene.set_effect(fixture.entity, instance);

  auto start = ura::AnimationClock::now();
  ura::EffectUniformTrack track {
    .name = "amount",
    .slot = 0,
    .type = ura::EffectUniformType::Float,
    .from = { { 0.f, 0.f, 0.f, 0.f } },
    .to = { { 1.f, 0.f, 0.f, 0.f } },
    .start = {},
    .options =
      ura::AnimationOptions {
        .duration = std::chrono::milliseconds(100),
        .easing = ura::EasingCurve::from_name("linear"),
      },
    .running = true,
  };
  fixture.animations.animate_effect_uniform(fixture.entity, track, start);
  if (!fixture.animations.has_active())
    return false;

  auto value = [&] {
    auto* shader_effect =
      fixture.scene.try_get<ura::ShaderEffect>(fixture.entity);
    if (!shader_effect)
      return -1.f;
    for (auto& uniform : shader_effect->effect.uniforms) {
      if (uniform.name == "amount")
        return uniform.values[0];
    }
    return -1.f;
  };

  if (!near(value(), 0.0))
    return false;
  fixture.animations.tick(start + std::chrono::milliseconds(50));
  if (!near(value(), 0.5))
    return false;
  fixture.animations.tick(start + std::chrono::milliseconds(100));
  if (!near(value(), 1.0) || fixture.animations.has_active())
    return false;

  // Non-continuous uniforms are rejected by the public API layer; the
  // animation system itself uses the track type only for vector width.
  return true;
}

bool test_transition_inherits_move_animation() {
  ura::WindowTransition transition;
  transition.options.duration = std::chrono::milliseconds(0);

  auto move = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(200),
    .easing = ura::EasingCurve::from_name("out_cubic"),
  };
  transition.inherit(move);
  if (transition.options.duration != std::chrono::milliseconds(200))
    return false;

  auto shorter = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(50),
    .easing = ura::EasingCurve::from_name("linear"),
  };
  transition.inherit(shorter);
  return transition.options.duration == std::chrono::milliseconds(200)
    && transition.options.easing.sample(0.5) == move.easing.sample(0.5);
}

bool test_destroy_on_animation_finish() {
  Fixture fixture;
  auto start = ura::AnimationClock::now();
  bool destroyed = false;
  fixture.animations.set_destroy_callback([&](entt::entity entity) {
    destroyed = true;
    fixture.scene.destroy(entity);
  });

  auto options = ura::AnimationOptions {
    .duration = std::chrono::milliseconds(20),
    .easing = ura::EasingCurve::from_name("linear"),
  };
  fixture.animations.animate_from(
    fixture.entity,
    ura::Rect<double> { .x = 0, .y = 0, .width = 100, .height = 100 },
    ura::Rect<double> { .x = 20, .y = 0, .width = 100, .height = 100 },
    options,
    start
  );
  auto* animation = fixture.scene.try_get<ura::WindowAnimation>(fixture.entity);
  if (!animation)
    return false;
  animation->destroy_on_finish = true;

  fixture.animations.tick(start + std::chrono::milliseconds(20));
  if (destroyed)
    return false;
  fixture.animations.tick(start + std::chrono::milliseconds(21));
  return destroyed && !fixture.entities.contains(fixture.entity);
}

} // namespace

int main(int argc, char** argv) {
  auto test = argc > 1 ? std::string(argv[1]) : std::string {};
  if (test == "easing")
    return test_easing_curve() ? 0 : 1;
  if (test == "data-boundary")
    return test_layout_content_and_node_are_separate() ? 0 : 1;
  if (test == "flip")
    return test_flip_interpolation_and_finish() ? 0 : 1;
  if (test == "retarget")
    return test_retarget_from_current_visual() ? 0 : 1;
  if (test == "timeout")
    return test_resize_timeout_uses_committed_size() ? 0 : 1;
  if (test == "opacity")
    return test_opacity_only_animation() ? 0 : 1;
  if (test == "opacity-retarget")
    return test_opacity_survives_retarget() ? 0 : 1;
  if (test == "effect-uniform")
    return test_effect_uniform_animation() ? 0 : 1;
  if (test == "rotation")
    return test_rotation_animation() ? 0 : 1;
  if (test == "transition-move-merge")
    return test_transition_inherits_move_animation() ? 0 : 1;
  if (test == "destroy-on-finish")
    return test_destroy_on_animation_finish() ? 0 : 1;

  std::cerr << "unknown test: " << test << '\n';
  return 2;
}
