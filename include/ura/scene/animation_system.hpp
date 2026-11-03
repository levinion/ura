#pragma once

#include <entt/entt.hpp>

#include <functional>

#include "ura/scene/animation.hpp"
#include "ura/scene/components.hpp"

namespace ura {

class UraScene;

class UraAnimationSystem {
public:
  explicit UraAnimationSystem(UraScene& scene);

  // Animate from the current visible rectangle to `to`.
  void animate(
    entt::entity entity,
    const Rect<double>& to,
    const AnimationOptions& options,
    AnimationTimePoint now = AnimationClock::now()
  );

  void animate_from(
    entt::entity entity,
    const Rect<double>& from,
    const Rect<double>& to,
    const AnimationOptions& options,
    AnimationTimePoint now = AnimationClock::now()
  );

  // Freeze a running animation at its current sampled value.
  void
  settle(entt::entity entity, AnimationTimePoint now = AnimationClock::now());

  // Stop immediately and place the visual state at `to`.
  void snap(entt::entity entity, const Rect<double>& to);

  // Drop the animation without touching the visual state.
  void cancel(entt::entity entity);

  // Add or replace one shader-uniform ramp. The C++ animation system samples
  // it; Lua is only involved when the track is created.
  void animate_effect_uniform(
    entt::entity entity,
    const EffectUniformTrack& track,
    AnimationTimePoint now = AnimationClock::now()
  );
  void cancel_effect_uniforms(entt::entity entity);

  bool running(entt::entity entity) const;
  bool active(entt::entity entity) const;
  // Current sampled rectangle of a running animation, or the current
  // SceneNode bounds when no animation is active.
  Rect<double> sample(
    entt::entity entity,
    AnimationTimePoint now = AnimationClock::now()
  ) const;
  bool has_active() const;

  // Called for detached content nodes whose animation is marked
  // destroy_on_finish. The view uses it to release the render target and then
  // destroy the scene entity.
  void set_destroy_callback(std::function<void(entt::entity)> callback);

  // Samples running animations and starts timed-out commit-first transitions.
  // Returns true when scene state was changed.
  bool tick(AnimationTimePoint now);

private:
  UraScene& scene;
  std::function<void(entt::entity)> destroy_callback;

  Rect<double> layout_target(entt::entity entity) const;
  void apply_bounds(entt::entity entity, const Rect<double>& bounds);
  void start_timed_out_transition(
    entt::entity entity,
    WindowTransition& transition,
    AnimationTimePoint now
  );
  void finish_transition(entt::entity entity);
  bool apply_effect_uniform(
    entt::entity entity,
    const EffectUniformTrack& track,
    const std::array<float, 4>& values
  );
};

} // namespace ura
