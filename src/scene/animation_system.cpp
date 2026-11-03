#include "ura/scene/animation_system.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "ura/scene/scene.hpp"

namespace ura {

UraAnimationSystem::UraAnimationSystem(UraScene& scene) : scene(scene) {}

Rect<double> UraAnimationSystem::layout_target(entt::entity entity) const {
  auto* layout = this->scene.try_get<WindowLayout>(entity);
  auto* content = this->scene.try_get<SurfaceContent>(entity);
  if (!layout || !content)
    return {};
  return layout->resolve(*content);
}

void UraAnimationSystem::apply_bounds(
  entt::entity entity,
  const Rect<double>& bounds
) {
  this->scene.set_node_bounds(entity, bounds);
}

void UraAnimationSystem::finish_transition(entt::entity entity) {
  auto* transition = this->scene.try_get<WindowTransition>(entity);
  if (transition && transition->phase == WindowTransitionPhase::Animating)
    transition->phase = WindowTransitionPhase::Idle;
}

void UraAnimationSystem::start_timed_out_transition(
  entt::entity entity,
  WindowTransition& transition,
  AnimationTimePoint now
) {
  auto target = this->layout_target(entity);
  if (target.empty())
    target = transition.requested_target;

  transition.resolved_target = target;
  transition.phase = WindowTransitionPhase::Animating;
  this->animate_from(entity, transition.from, target, transition.options, now);
}

void UraAnimationSystem::animate(
  entt::entity entity,
  const Rect<double>& to,
  const AnimationOptions& options,
  AnimationTimePoint now
) {
  this->settle(entity, now);
  this->animate_from(entity, this->scene.node_bounds(entity), to, options, now);
}

void UraAnimationSystem::animate_from(
  entt::entity entity,
  const Rect<double>& from,
  const Rect<double>& to,
  const AnimationOptions& options,
  AnimationTimePoint now
) {
  auto& animation = this->scene.ensure_window_animation(entity);
  auto was_animating_opacity = animation.carry_opacity
    || (animation.running && animation.options.animate_opacity);
  auto previous_to_opacity = animation.to_opacity;
  animation.carry_opacity = false;
  auto was_animating_rotation = animation.carry_rotation
    || (animation.running && animation.options.animate_rotation);
  auto previous_to_rotation = animation.to_rotation;
  animation.carry_rotation = false;

  animation.generation++;
  animation.from = from;
  animation.to = to;
  animation.start = now;
  animation.options = options;

  if (auto* opacity = this->scene.try_get<Opacity>(entity)) {
    animation.from_opacity = opacity->value;
    if (options.animate_opacity) {
      animation.to_opacity = std::clamp(options.opacity, 0.f, 1.f);
    } else if (was_animating_opacity) {
      // A layout retarget during a window-open/close effect must not cancel
      // the independent opacity part of the animation.
      animation.options.animate_opacity = true;
      animation.to_opacity = previous_to_opacity;
    } else {
      animation.to_opacity = opacity->value;
    }
  } else {
    animation.from_opacity = 1.f;
    animation.to_opacity = 1.f;
    animation.options.animate_opacity = false;
  }

  if (auto* node = this->scene.try_get<SceneNode>(entity)) {
    animation.from_rotation = node->rotation;
    if (options.animate_rotation) {
      animation.to_rotation = options.rotation;
    } else if (was_animating_rotation) {
      animation.options.animate_rotation = true;
      animation.to_rotation = previous_to_rotation;
    } else {
      animation.to_rotation = node->rotation;
    }
  } else {
    animation.from_rotation = 0.f;
    animation.to_rotation = 0.f;
    animation.options.animate_rotation = false;
  }

  auto opacity_changes = animation.options.animate_opacity
    && std::abs(animation.to_opacity - animation.from_opacity) > 1e-6f;
  auto rotation_changes = animation.options.animate_rotation
    && std::abs(animation.to_rotation - animation.from_rotation) > 1e-6f;
  if (
    options.duration <= std::chrono::milliseconds::zero()
    || (from == to && !opacity_changes && !rotation_changes)
  ) {
    animation.running = false;
    this->apply_bounds(entity, to);
    if (animation.options.animate_opacity)
      this->scene.set_opacity(entity, animation.to_opacity);
    if (animation.options.animate_rotation)
      this->scene.set_node_rotation(entity, animation.to_rotation);
    this->finish_transition(entity);
    return;
  }

  animation.running = true;
  // Pin SceneNode.bounds to the captured old rectangle immediately. This is
  // what prevents a jump when a new client size was just committed.
  this->apply_bounds(entity, from);
}

void UraAnimationSystem::settle(entt::entity entity, AnimationTimePoint now) {
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  if (!animation || !animation->running)
    return;

  auto duration =
    std::chrono::duration_cast<AnimationDuration>(animation->options.duration);
  double progress = 1.0;
  if (duration > AnimationDuration::zero()) {
    auto elapsed = now - animation->start;
    progress = std::clamp(
      static_cast<double>(elapsed.count())
        / static_cast<double>(duration.count()),
      0.0,
      1.0
    );
  }

  auto eased = static_cast<double>(animation->options.easing.sample(progress));
  this->apply_bounds(entity, lerp(animation->from, animation->to, eased));
  if (animation->options.animate_opacity) {
    auto opacity = animation->from_opacity
      + (animation->to_opacity - animation->from_opacity)
        * static_cast<float>(eased);
    this->scene.set_opacity(entity, opacity);
  }
  if (animation->options.animate_rotation) {
    auto rotation = animation->from_rotation
      + (animation->to_rotation - animation->from_rotation)
        * static_cast<float>(eased);
    this->scene.set_node_rotation(entity, rotation);
  }

  animation->carry_opacity = animation->options.animate_opacity;
  animation->carry_rotation = animation->options.animate_rotation;
  animation->running = false;
  this->finish_transition(entity);
}

void UraAnimationSystem::snap(entt::entity entity, const Rect<double>& to) {
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  if (animation) {
    animation->running = false;
    animation->carry_opacity = false;
  }
  this->apply_bounds(entity, to);
  this->finish_transition(entity);
}

void UraAnimationSystem::cancel(entt::entity entity) {
  this->cancel_effect_uniforms(entity);
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  if (!animation)
    return;
  animation->running = false;
  animation->carry_opacity = false;
  animation->carry_rotation = false;
  animation->generation++;
}

void UraAnimationSystem::animate_effect_uniform(
  entt::entity entity,
  const EffectUniformTrack& track,
  AnimationTimePoint now
) {
  if (
    track.type == EffectUniformType::Int
    || track.type == EffectUniformType::Bool
  )
    return;

  auto& animation = this->scene.ensure_effect_uniform_animation(entity);
  std::erase_if(animation.tracks, [&](const EffectUniformTrack& candidate) {
    return candidate.name == track.name;
  });

  auto next = track;
  next.start = now;
  if (
    next.options.duration <= std::chrono::milliseconds::zero()
    || next.from == next.to
  ) {
    this->apply_effect_uniform(entity, next, next.to);
    return;
  }

  if (!this->apply_effect_uniform(entity, next, next.from))
    return;
  animation.tracks.push_back(std::move(next));
}

void UraAnimationSystem::cancel_effect_uniforms(entt::entity entity) {
  auto* animation = this->scene.try_get<EffectUniformAnimation>(entity);
  if (!animation)
    return;
  animation->tracks.clear();
}

bool UraAnimationSystem::apply_effect_uniform(
  entt::entity entity,
  const EffectUniformTrack& track,
  const std::array<float, 4>& values
) {
  auto* shader_effect = this->scene.try_get<ShaderEffect>(entity);
  if (!shader_effect || !shader_effect->effect.active())
    return false;

  auto effect = shader_effect->effect;
  if (track.slot >= effect.uniforms.size())
    return false;
  auto& uniform = effect.uniforms[track.slot];
  if (uniform.type != track.type)
    return false;
  if (uniform.values == values)
    return true;
  uniform.values = values;
  effect.version++;
  this->scene.set_effect(entity, effect);
  return true;
}

bool UraAnimationSystem::running(entt::entity entity) const {
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  return animation && animation->running;
}

bool UraAnimationSystem::active(entt::entity entity) const {
  if (this->running(entity))
    return true;
  auto* transition = this->scene.try_get<WindowTransition>(entity);
  return transition
    && transition->phase == WindowTransitionPhase::WaitingForCommit;
}

Rect<double>
UraAnimationSystem::sample(entt::entity entity, AnimationTimePoint now) const {
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  if (!animation || !animation->running)
    return this->scene.node_bounds(entity);

  auto duration =
    std::chrono::duration_cast<AnimationDuration>(animation->options.duration);
  double progress = 1.0;
  if (duration > AnimationDuration::zero()) {
    auto elapsed = now - animation->start;
    progress = std::clamp(
      static_cast<double>(elapsed.count())
        / static_cast<double>(duration.count()),
      0.0,
      1.0
    );
  }
  auto eased = static_cast<double>(animation->options.easing.sample(progress));
  return lerp(animation->from, animation->to, eased);
}

bool UraAnimationSystem::has_active() const {
  auto& registry = this->scene.raw();
  auto animations = registry.view<WindowAnimation>();
  for (auto [entity, animation] : animations.each()) {
    if (animation.running)
      return true;
  }

  auto effect_animations = registry.view<EffectUniformAnimation>();
  for (auto [entity, animation] : effect_animations.each()) {
    if (!animation.tracks.empty())
      return true;
  }

  auto transitions = registry.view<WindowTransition>();
  for (auto [entity, transition] : transitions.each()) {
    if (transition.phase == WindowTransitionPhase::WaitingForCommit)
      return true;
  }
  return false;
}

bool UraAnimationSystem::tick(AnimationTimePoint now) {
  bool changed = false;
  auto& registry = this->scene.raw();

  std::vector<entt::entity> timed_out;
  auto transitions = registry.view<WindowTransition>();
  for (auto [entity, transition] : transitions.each()) {
    if (
      transition.phase == WindowTransitionPhase::WaitingForCommit
      && now >= transition.deadline
    ) {
      timed_out.push_back(entity);
    }
  }
  for (auto entity : timed_out) {
    auto* transition = this->scene.try_get<WindowTransition>(entity);
    if (
      !transition
      || transition->phase != WindowTransitionPhase::WaitingForCommit
      || now < transition->deadline
    )
      continue;
    this->start_timed_out_transition(entity, *transition, now);
    changed = true;
  }

  std::vector<entt::entity> finished;
  auto animations = registry.view<WindowAnimation>();
  for (auto [entity, animation] : animations.each()) {
    if (animation.destroy_pending && !animation.running) {
      finished.push_back(entity);
      continue;
    }
    if (!animation.running)
      continue;

    auto duration =
      std::chrono::duration_cast<AnimationDuration>(animation.options.duration);
    double progress = 1.0;
    if (duration > AnimationDuration::zero()) {
      auto elapsed = now - animation.start;
      progress = std::clamp(
        static_cast<double>(elapsed.count())
          / static_cast<double>(duration.count()),
        0.0,
        1.0
      );
    }

    auto eased = static_cast<double>(animation.options.easing.sample(progress));
    this->apply_bounds(entity, lerp(animation.from, animation.to, eased));
    if (animation.options.animate_opacity) {
      auto opacity = animation.from_opacity
        + (animation.to_opacity - animation.from_opacity)
          * static_cast<float>(eased);
      this->scene.set_opacity(entity, opacity);
    }
    if (animation.options.animate_rotation) {
      auto rotation = animation.from_rotation
        + (animation.to_rotation - animation.from_rotation)
          * static_cast<float>(eased);
      this->scene.set_node_rotation(entity, rotation);
    }
    changed = true;

    if (progress >= 1.0) {
      animation.running = false;
      animation.carry_opacity = false;
      animation.carry_rotation = false;
      this->finish_transition(entity);
      if (animation.destroy_on_finish) {
        animation.destroy_on_finish = false;
        animation.destroy_pending = true;
      }
    }
  }

  auto effect_animations = registry.view<EffectUniformAnimation>();
  for (auto [entity, animation] : effect_animations.each()) {
    for (auto it = animation.tracks.begin(); it != animation.tracks.end();) {
      auto& track = *it;
      if (!track.running) {
        it = animation.tracks.erase(it);
        continue;
      }

      auto duration =
        std::chrono::duration_cast<AnimationDuration>(track.options.duration);
      double progress = 1.0;
      if (duration > AnimationDuration::zero()) {
        auto elapsed = now - track.start;
        progress = std::clamp(
          static_cast<double>(elapsed.count())
            / static_cast<double>(duration.count()),
          0.0,
          1.0
        );
      }

      auto eased = static_cast<double>(track.options.easing.sample(progress));
      auto values = track.from;
      auto count = track.type == EffectUniformType::Vec4 ? 4
        : track.type == EffectUniformType::Vec3          ? 3
        : track.type == EffectUniformType::Vec2          ? 2
                                                         : 1;
      for (int i = 0; i < count; ++i) {
        values[i] = static_cast<float>(
          track.from[i] + (track.to[i] - track.from[i]) * eased
        );
      }

      if (!this->apply_effect_uniform(entity, track, values)) {
        it = animation.tracks.erase(it);
        continue;
      }
      changed = true;

      if (progress >= 1.0) {
        it = animation.tracks.erase(it);
      } else {
        ++it;
      }
    }
  }

  for (auto entity : finished) {
    if (this->destroy_callback) {
      this->destroy_callback(entity);
      continue;
    }
    if (this->scene.raw().valid(entity)) {
      this->scene.raw().destroy(entity);
      this->scene.invalidate();
    }
  }

  return changed;
}

void UraAnimationSystem::set_destroy_callback(
  std::function<void(entt::entity)> callback
) {
  this->destroy_callback = std::move(callback);
}

} // namespace ura
