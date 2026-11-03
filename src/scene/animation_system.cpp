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

void UraAnimationSystem::apply_bounds(entt::entity entity, const Rect<double>& bounds) {
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

  auto opacity_changes = animation.options.animate_opacity
    && std::abs(animation.to_opacity - animation.from_opacity) > 1e-6f;
  if (
    options.duration <= std::chrono::milliseconds::zero()
    || (from == to && !opacity_changes)
  ) {
    animation.running = false;
    this->apply_bounds(entity, to);
    if (animation.options.animate_opacity)
      this->scene.set_opacity(entity, animation.to_opacity);
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

  animation->carry_opacity = animation->options.animate_opacity;
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
  auto* animation = this->scene.try_get<WindowAnimation>(entity);
  if (!animation)
    return;
  animation->running = false;
  animation->carry_opacity = false;
  animation->generation++;
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

Rect<double> UraAnimationSystem::sample(
  entt::entity entity,
  AnimationTimePoint now
) const {
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
    changed = true;

    if (progress >= 1.0) {
      animation.running = false;
      animation.carry_opacity = false;
      this->finish_transition(entity);
      if (animation.destroy_on_finish) {
        animation.destroy_on_finish = false;
        animation.destroy_pending = true;
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
