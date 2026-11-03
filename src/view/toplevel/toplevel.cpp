#include "ura/view/toplevel.hpp"

#include <cmath>

#include "ura/scene/animation_system.hpp"
#include "ura/scene/layer.hpp"
#include "ura/util/flexible.hpp"
#include "ura/ura.hpp"
#include "ura/util/rgb.hpp"
#include "ura/util/vec.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/core/callback.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/lua.hpp"
#include "ura/view/view.hpp"

namespace ura {

namespace {

  Rect<double>
  scale_rect_around_center(const Rect<double>& bounds, float scale) {
    if (scale == 1.f || bounds.empty())
      return bounds;
    auto width = bounds.width * static_cast<double>(scale);
    auto height = bounds.height * static_cast<double>(scale);
    return Rect<double> {
      .x = bounds.x + (bounds.width - width) / 2.0,
      .y = bounds.y + (bounds.height - height) / 2.0,
      .width = width,
      .height = height,
    };
  }

} // namespace

UraScene& UraToplevel::scene_model() {
  return *UraServer::get_instance()->view->scene;
}

const WindowLayout& UraToplevel::layout() {
  return this->scene_model().get<WindowLayout>(this->entity);
}

const SurfaceContent& UraToplevel::content() {
  return this->scene_model().get<SurfaceContent>(this->entity);
}

const SceneNode& UraToplevel::node() {
  return this->scene_model().get<SceneNode>(this->entity);
}

const Visibility& UraToplevel::visibility() {
  return this->scene_model().get<Visibility>(this->entity);
}

const ZOrder& UraToplevel::z_order() {
  return this->scene_model().get<ZOrder>(this->entity);
}

const WindowTags& UraToplevel::tags() {
  return this->scene_model().get<WindowTags>(this->entity);
}

const Opacity& UraToplevel::opacity() {
  return this->scene_model().get<Opacity>(this->entity);
}

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  this->xdg_toplevel = xdg_toplevel;
  this->cached_title = xdg_toplevel->title ? xdg_toplevel->title : "";
  this->cached_app_id = xdg_toplevel->app_id ? xdg_toplevel->app_id : "";
  this->entity = server->entities->create_toplevel(this);

  auto output = server->view->current_output();
  WindowTags tags;
  if (output)
    tags.values.assign(output->tags.begin(), output->tags.end());

  server->view->scene->attach_toplevel(
    this->entity,
    Rect<double> {},
    std::move(tags),
    ZOrder { UraSceneLayer::Normal, 0 }
  );

  xdg_toplevel->base->surface->data = this;
  this->set_scale(this->scale());

  server->view->toplevels.push_back(this);

  // register callback
  {
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.map,
      on_toplevel_map,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.unmap,
      on_toplevel_unmap,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.commit,
      on_toplevel_commit,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.destroy,
      on_toplevel_destroy,
      this
    );
    // server->runtime->register_callback(
    //   &xdg_toplevel->events.request_resize,
    //   on_toplevel_request_resize,
    //   toplevel
    // );
    server->runtime->register_callback(
      &xdg_toplevel->events.request_maximize,
      on_toplevel_request_maximize,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.request_fullscreen,
      on_toplevel_request_fullscreen,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.set_app_id,
      on_toplevel_set_app_id,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.set_title,
      on_toplevel_set_title,
      this
    );
  }

  // forign toplevel handle
  this->foreign_handle =
    wlr_foreign_toplevel_handle_v1_create(server->foreign_manager);
  this->foreign_handle->data = this;

  server->runtime->register_callback(
    &this->foreign_handle->events.request_fullscreen,
    on_foreign_toplevel_handle_request_fullscreen,
    this
  );
  server->runtime->register_callback(
    &this->foreign_handle->events.request_activate,
    on_foreign_toplevel_handle_request_activate,
    this
  );
}

void UraToplevel::destroy() {
  auto server = UraServer::get_instance();
  this->destroying = true;
  this->cancel_transition_timer();
  for (auto* popup : server->seat->text_input->popups) {
    if (popup->parent == this)
      popup->parent = nullptr;
  }
  server->view->toplevels.remove(this);
  if (this->is_focused()) {
    server->seat->focus_lru();
  }
  server->runtime->remove(this);
  wlr_foreign_toplevel_handle_v1_destroy(this->foreign_handle);
  this->dismiss_popups();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-close", args);

  // Close animations are opt-in. A plugin requests one from the window-close
  // hook with ura.api.animate_window_close(); the request captured the final
  // committed content while it was still alive.
  auto* animations = server->view->animations.get();
  auto close_target = this->pending_close_target;
  auto close_bounds = this->pending_close_bounds;
  auto close_options = this->pending_close_options;
  auto close_scale = this->pending_close_scale;
  auto close_opacity = this->pending_close_opacity;

  if (animations && close_target != 0 && close_options.duration.count() > 0) {
    auto ghost = server->entities->create_scene_node();
    auto& registry = server->entities->raw();
    auto& scene = this->scene_model();
    registry.emplace_or_replace<WindowLayout>(ghost, this->layout());
    registry.emplace_or_replace<SurfaceContent>(ghost, this->content());
    auto node = this->node();
    node.bounds = close_bounds;
    registry.emplace_or_replace<SceneNode>(ghost, node);
    registry.emplace_or_replace<Visibility>(
      ghost,
      Visibility { .visible = true, .occluded = false }
    );
    registry.emplace_or_replace<ZOrder>(ghost, this->z_order());
    registry.emplace_or_replace<Opacity>(ghost, this->opacity());
    registry.emplace_or_replace<ShaderEffect>(
      ghost,
      ShaderEffect { .effect = this->effect() }
    );
    registry.emplace_or_replace<RenderTarget>(
      ghost,
      RenderTarget { .resource = close_target }
    );
    registry.emplace_or_replace<WindowTransition>(ghost, WindowTransition {});
    registry.emplace_or_replace<WindowAnimation>(ghost, WindowAnimation {});
    scene.invalidate();

    auto from = close_bounds;
    auto to = scale_rect_around_center(from, close_scale);
    auto options = close_options;
    options.animate_opacity = true;
    options.opacity = close_opacity;
    animations->animate_from(ghost, from, to, options);
    if (
      auto* animation = registry.try_get<WindowAnimation>(ghost);
      animation && animation->running
    ) {
      animation->destroy_on_finish = true;
    } else {
      server->view->destroy_scene_entity(ghost);
    }
  } else if (close_target != 0) {
    server->view->release_render_target(close_target);
  }
  this->pending_close_target = 0;

  // Keep the entity resolvable while window-close runs, then invalidate it.
  server->entities->destroy(this->entity);
  this->entity = entt::null;
  server->view->schedule_frames();
}

void UraToplevel::commit() {
  auto server = UraServer::get_instance();
  UraOutput* output = nullptr;
  if (
    this->xdg_toplevel->requested.fullscreen
    && this->xdg_toplevel->requested.fullscreen_output
  ) {
    output = UraOutput::from(this->xdg_toplevel->requested.fullscreen_output);
  }
  if (!output)
    output = this->output();
  if (!output)
    output = server->view->current_output();
  if (!output || !this->xdg_toplevel->base->initialized) {
    return;
  }
  this->set_output(output);
  auto previous_content = this->content();
  this->update_surface_content_from_surface();
  auto content_changed = previous_content != this->content();
  // first commit
  if (this->xdg_toplevel->base->initial_commit) {
    if (this->decoration)
      wlr_xdg_toplevel_decoration_v1_set_mode(
        this->decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
      );
    // let the client to decide its size
    if (
      this->xdg_toplevel->base->current.geometry.width == 0
      || this->xdg_toplevel->base->current.geometry.height == 0
    ) {
      wlr_xdg_toplevel_set_size(this->xdg_toplevel, 0, 0);
      return;
    }
  }

  // second commit
  if (!this->prepared) {
    // set a default size if the given size is invalid
    if (
      this->xdg_toplevel->base->current.geometry.width == 0
      || this->xdg_toplevel->base->current.geometry.height == 0
    ) {
      this->resize(800, 600);
    } else {
      auto target = this->layout().target;
      auto content = this->content();
      if (!content.empty()) {
        target.width = static_cast<double>(content.geometry.width);
        target.height = static_cast<double>(content.geometry.height);
      } else {
        target.width =
          static_cast<double>(this->xdg_toplevel->base->current.geometry.width);
        target.height = static_cast<double>(
          this->xdg_toplevel->base->current.geometry.height
        );
      }
      server->view->scene->set_layout_target(this->entity, target);
    }
    if (this->xdg_toplevel->requested.fullscreen) {
      auto geo = output->logical_geometry();
      this->resize(geo.width, geo.height);
      this->move(geo.x, geo.y);
    } else if (this->xdg_toplevel->requested.maximized) {
      auto geo = output->usable_area;
      this->resize(geo.width, geo.height);
      this->move(geo.x, geo.y);
    } else {
      this->center();
    }
    this->prepared = true;
    this->apply_pending_fullscreen_output();
    this->scene_model().sync_scene_node(this->entity);

    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("window-new", args);
  }

  auto started_transition = this->check_transition_commit();
  if (content_changed && !started_transition && this->prepared) {
    auto* transition =
      this->scene_model().try_get<WindowTransition>(this->entity);
    auto waiting = transition
      && transition->phase == WindowTransitionPhase::WaitingForCommit;
    if (!waiting) {
      auto* animation =
        this->scene_model().try_get<WindowAnimation>(this->entity);
      auto* animations = server->view->animations.get();
      if (animation && animation->running && animations) {
        auto now = AnimationClock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          now - animation->start
        );
        auto remaining = animation->options.duration - elapsed;
        if (remaining < std::chrono::milliseconds::zero())
          remaining = std::chrono::milliseconds::zero();
        auto options = animation->options;
        options.duration = remaining;
        animations->animate_from(
          this->entity,
          animations->sample(this->entity, now),
          this->layout_target_rect(),
          options,
          now
        );
      } else {
        this->scene_model().sync_scene_node(this->entity);
      }
    }
  }

  // update opacity
  this->set_opacity(this->opacity().value);
}

void UraToplevel::focus() {
  if (
    !this->xdg_toplevel->base->initialized || this->is_focused()
    || !this->mapped()
  )
    return;

  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto surface = this->xdg_toplevel->base->surface;

  server->view->scene->raise(this->entity);
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
  auto keyboard = wlr_seat_get_keyboard(seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      seat,
      surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
  server->seat->text_input->focus_text_input(surface);
  this->lru = std::chrono::steady_clock::now().time_since_epoch().count();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-focus", args);
}

void UraToplevel::unfocus() {
  if (!this->xdg_toplevel->base->initialized)
    return;
  if (!this->is_focused())
    return;

  if (!this->destroying) {
    wlr_xdg_toplevel_set_activated(this->xdg_toplevel, false);
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
  }

  auto server = UraServer::get_instance();
  server->seat->text_input->unfocus_active_text_input();

  this->dismiss_popups();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-unfocus", args);
}

// get toplevel instance from wlr_surface, it asserts the surface's role is xdg_toplevel
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

UraToplevel* UraToplevel::from(uint64_t id) {
  return UraServer::get_instance()->entities->toplevel(id);
}

void UraToplevel::activate() {
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());

  auto results =
    server->lua->emit_hook<std::vector<bool>>("pre-window-activate", args);
  if (
    results
    && std::find(results->begin(), results->end(), false) != results->end()
  )
    return;

  auto output = this->output();
  if (!output)
    return;
  auto tags = this->tags().values;
  output->set_tags(Vec<std::string>(tags.begin(), tags.end()));
  server->seat->focus(this);
  server->lua->emit_hook("window-activate", args);
}

bool UraToplevel::move(int x, int y) {
  return this->move(x, y, AnimationOptions {});
}

bool UraToplevel::move(int x, int y, const AnimationOptions& options) {
  auto target = this->layout().target;
  if (x == target.x && y == target.y)
    return false;

  auto old_output = this->prepared ? this->output() : nullptr;
  target.x = static_cast<double>(x);
  target.y = static_cast<double>(y);
  this->scene_model().set_layout_target(this->entity, target);
  if (this->prepared) {
    auto new_output = this->output();
    this->set_output(new_output);
    if (old_output != new_output) {
      if (this->is_tag_matched()) {
        this->map();
      } else {
        auto refocus = this->is_focused();
        this->unmap();
        if (refocus)
          UraServer::get_instance()->seat->focus_lru();
      }
    }
  }

  auto server = UraServer::get_instance();
  auto* scene = &this->scene_model();
  if (this->prepared && this->visibility().visible) {
    auto* transition = scene->try_get<WindowTransition>(this->entity);
    if (
      transition && transition->phase == WindowTransitionPhase::WaitingForCommit
    ) {
      // A resize is waiting for the client. Keep the old visual state but
      // remember the new layout anchor; the FLIP animation will use it once
      // the matching surface commit arrives.
      transition->requested_target.x = static_cast<double>(x);
      transition->requested_target.y = static_cast<double>(y);

      // Layout plugins usually call resize() and move() together. If only the
      // move rule is animated, the resize transition must inherit its timing;
      // otherwise the commit would complete with a zero-duration snap.
      transition->inherit(options);
    } else if (auto* animations = server->view->animations.get()) {
      animations->settle(this->entity);
      auto from = scene->node_bounds(this->entity);
      auto content = this->content();
      auto to = Rect<double> {
        .x = static_cast<double>(x),
        .y = static_cast<double>(y),
        .width = static_cast<double>(content.geometry.width),
        .height = static_cast<double>(content.geometry.height),
      };
      if (to.empty()) {
        scene->sync_scene_node(this->entity);
      } else {
        animations->animate_from(this->entity, from, to, options);
      }
    }
  } else {
    scene->sync_scene_node(this->entity);
  }

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-move", args);

  return true;
}

bool UraToplevel::resize(int width, int height) {
  return this->resize(width, height, AnimationOptions {});
}

bool UraToplevel::resize(
  int width,
  int height,
  const AnimationOptions& options
) {
  if (!this->xdg_toplevel->base->initialized)
    return false;

  if (width <= 0 || height <= 0)
    return false;

  // if (this->xdg_toplevel->current.max_width > 0) {
  //   width = std::min(width, this->xdg_toplevel->current.max_width);
  // }
  if (this->xdg_toplevel->current.min_width > 0) {
    width = std::max(width, this->xdg_toplevel->current.min_width);
  }
  // if (this->xdg_toplevel->current.max_height > 0) {
  //   height = std::min(height, this->xdg_toplevel->current.max_height);
  // }
  if (this->xdg_toplevel->current.min_height > 0) {
    height = std::max(height, this->xdg_toplevel->current.min_height);
  }

  auto target = this->layout().target;
  if (
    width == static_cast<int>(target.width)
    && height == static_cast<int>(target.height)
  )
    return false;

  auto server = UraServer::get_instance();
  auto* scene = &this->scene_model();
  auto* transition = scene->try_get<WindowTransition>(this->entity);
  auto previous_request =
    transition ? transition->requested_target : Rect<double> {};
  auto was_waiting =
    transition && transition->phase == WindowTransitionPhase::WaitingForCommit;
  auto same_pending_size = was_waiting
    && static_cast<int>(previous_request.width) == width
    && static_cast<int>(previous_request.height) == height;

  target.width = static_cast<double>(width);
  target.height = static_cast<double>(height);
  scene->set_layout_target(this->entity, target);

  if (!this->prepared || !this->visibility().visible) {
    this->cancel_transition_timer();
    if (transition) {
      transition->phase = WindowTransitionPhase::Idle;
      transition->generation++;
    }
    wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
    scene->sync_scene_node(this->entity);

    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("window-resize", args);
    return true;
  }

  if (!transition)
    transition = &scene->ensure_window_transition(this->entity);

  auto* animations = server->view->animations.get();
  if (animations)
    animations->settle(this->entity);

  auto from = scene->node_bounds(this->entity);
  auto layout = this->layout();
  auto content = this->content();

  transition->generation++;
  transition->from = from;
  transition->resolved_target = {};
  transition->requested_target = Rect<double> {
    .x = layout.target.x,
    .y = layout.target.y,
    .width = static_cast<double>(width),
    .height = static_cast<double>(height),
  };
  transition->options = options;

  auto size_matches_content =
    content.geometry.width == width && content.geometry.height == height;
  auto needs_configure =
    !size_matches_content || (was_waiting && !same_pending_size);

  if (!needs_configure) {
    transition->phase = WindowTransitionPhase::Idle;
    transition->resolved_target = Rect<double> {
      .x = layout.target.x,
      .y = layout.target.y,
      .width = static_cast<double>(content.geometry.width),
      .height = static_cast<double>(content.geometry.height),
    };
    this->cancel_transition_timer();
    if (animations) {
      animations->animate_from(
        this->entity,
        from,
        transition->resolved_target,
        options
      );
    }
  } else {
    transition->phase = WindowTransitionPhase::WaitingForCommit;
    if (!same_pending_size) {
      transition->base_configure_serial =
        this->xdg_toplevel->base->current.configure_serial;
      transition->base_seq = this->xdg_toplevel->base->surface->current.seq;
      transition->configure_serial =
        wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
    }
    transition->deadline =
      AnimationClock::now() + std::chrono::milliseconds(250);
    this->schedule_transition_timeout(transition->generation);
  }

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-resize", args);
  return true;
}

void UraToplevel::animate_opacity(
  float opacity,
  const AnimationOptions& options
) {
  auto opacity_value = std::clamp(opacity, 0.f, 1.f);
  auto server = UraServer::get_instance();
  auto* scene = &this->scene_model();
  if (!this->prepared || !this->visibility().visible) {
    scene->set_opacity(this->entity, opacity_value);
    return;
  }

  auto* animations = server->view->animations.get();
  if (!animations) {
    scene->set_opacity(this->entity, opacity_value);
    return;
  }

  auto animation_options = options;
  animation_options.animate_opacity = true;
  animation_options.opacity = opacity_value;

  // Bounds stay at their current value; only Opacity is interpolated. The
  // animation system keeps the opacity transition alive if a layout change
  // retargets the bounds animation in the meantime.
  auto bounds = scene->node_bounds(this->entity);
  animations->animate_from(this->entity, bounds, bounds, animation_options);
}

void UraToplevel::animate_open(
  const AnimationOptions& options,
  float from_scale,
  float from_opacity,
  float to_scale,
  float to_opacity
) {
  auto server = UraServer::get_instance();
  auto* animations = server->view->animations.get();
  auto* scene = &this->scene_model();
  if (!animations || !this->prepared || !this->visibility().visible)
    return;

  auto base = scene->node_bounds(this->entity);
  if (base.empty())
    return;

  auto from = scale_rect_around_center(base, from_scale);
  auto to = scale_rect_around_center(base, to_scale);
  auto animation_options = options;
  animation_options.animate_opacity = true;
  animation_options.opacity = std::clamp(to_opacity, 0.f, 1.f);

  scene->set_opacity(this->entity, std::clamp(from_opacity, 0.f, 1.f));
  animations->animate_from(this->entity, from, to, animation_options);
}

void UraToplevel::request_close_animation(
  const AnimationOptions& options,
  float to_scale,
  float to_opacity
) {
  if (
    !this->prepared || !this->xdg_toplevel->base->surface
    || options.duration.count() <= 0
  )
    return;

  auto server = UraServer::get_instance();
  if (this->pending_close_target != 0)
    server->view->release_render_target(this->pending_close_target);

  auto content = this->content();
  auto handle = server->view->capture_surface(
    this->xdg_toplevel->base->surface,
    content.geometry.x,
    content.geometry.y
  );
  if (handle == 0)
    return;

  this->pending_close_target = handle;
  this->pending_close_bounds = this->node().bounds;
  this->pending_close_options = options;
  this->pending_close_scale = to_scale;
  this->pending_close_opacity = std::clamp(to_opacity, 0.f, 1.f);
}

void UraToplevel::close() {
  wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  this->set_output(nullptr);
}

void UraToplevel::map() {
  if (
    this->mapped() || !this->xdg_toplevel->base->surface->mapped
    || !this->is_tag_matched()
  )
    return;

  auto server = UraServer::get_instance();

  auto args = flexible::create_table();
  args.set("id", this->id());
  args.set("initial", this->initial_map);

  server->lua->emit_hook("pre-window-map", args);

  this->scene_model().set_visible(this->entity, true);
  this->scene_model().sync_scene_node(this->entity);
  if (this->pending_close_target != 0) {
    server->view->release_render_target(this->pending_close_target);
    this->pending_close_target = 0;
    this->pending_close_options = {};
    this->pending_close_bounds = {};
  }

  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);

  server->lua->emit_hook("window-map", args);

  this->initial_map = false;
}

void UraToplevel::unmap() {
  // The surface's mapped flag may already be false when the unmap signal is
  // emitted, so use our scene visibility as the source of truth here.
  if (!this->visibility().visible)
    return;

  auto server = UraServer::get_instance();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("pre-window-unmap", args);

  // Hidden windows must not keep a pending commit transition or a running
  // layout animation alive. They are re-synchronized on the next map/commit.
  this->cancel_transition_timer();
  if (
    auto* transition =
      this->scene_model().try_get<WindowTransition>(this->entity)
  ) {
    transition->phase = WindowTransitionPhase::Idle;
    transition->generation++;
  }
  if (auto* animations = server->view->animations.get())
    animations->cancel(this->entity);

  this->scene_model().set_visible(this->entity, false);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);

  if (server->seat->focused_toplevel() == this) {
    server->seat->unfocus();
    // The client may unmap as part of surface destruction before the
    // xdg_toplevel destroy event. Move keyboard focus immediately so a
    // closing/hidden window does not leave the seat without a focus.
    server->seat->focus_lru();
  }

  server->lua->emit_hook("window-unmap", args);
}

std::string UraToplevel::title() {
  return this->cached_title;
}

std::string UraToplevel::app_id() {
  return this->cached_app_id;
}

void UraToplevel::set_title(std::string title) {
  this->cached_title = title;
  wlr_foreign_toplevel_handle_v1_set_title(this->foreign_handle, title.data());
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-title-change", args);
}

void UraToplevel::set_app_id(std::string app_id) {
  this->cached_app_id = app_id;
  wlr_foreign_toplevel_handle_v1_set_app_id(
    this->foreign_handle,
    app_id.data()
  );
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-app_id-change", args);
}

void UraToplevel::set_z_index(int z_index) {
  if (this->z_order().layer == z_index)
    return;
  this->scene_model().set_layer(this->entity, z_index);
}

bool UraToplevel::is_focused() {
  auto server = UraServer::get_instance();
  return server->seat->focused_toplevel() == this;
}

void UraToplevel::center() {
  auto output = this->output();
  if (!output)
    return;
  auto area = this->output()->usable_area;
  auto target = this->layout().target;
  auto geo = Rect<int> {
    static_cast<int>(std::lround(target.x)),
    static_cast<int>(std::lround(target.y)),
    static_cast<int>(std::lround(target.width)),
    static_cast<int>(std::lround(target.height)),
  };
  geo.center(area);
  this->move(geo.x, geo.y);
}

Rect<double> UraToplevel::layout_target_rect() {
  return this->layout().resolve(this->content());
}

void UraToplevel::update_surface_content_from_surface() {
  auto* base = this->xdg_toplevel->base;
  auto geometry = base->geometry;
  if (geometry.width <= 0 || geometry.height <= 0)
    geometry = base->current.geometry;
  if (geometry.width <= 0 || geometry.height <= 0) {
    geometry.x = 0;
    geometry.y = 0;
    geometry.width = base->surface->current.width;
    geometry.height = base->surface->current.height;
  }
  if (geometry.width <= 0 || geometry.height <= 0)
    return;

  this->scene_model().set_surface_content(
    this->entity,
    SurfaceContent {
      .geometry =
        Rect<int> {
          .x = geometry.x,
          .y = geometry.y,
          .width = geometry.width,
          .height = geometry.height,
        },
    }
  );
}

bool UraToplevel::check_transition_commit() {
  auto* transition =
    this->scene_model().try_get<WindowTransition>(this->entity);
  if (
    !transition || transition->phase != WindowTransitionPhase::WaitingForCommit
    || !this->prepared
  )
    return false;

  auto* base = this->xdg_toplevel->base;
  auto acked = base->current.configure_serial;
  auto requested = transition->configure_serial;
  if (acked == 0 || requested == 0)
    return false;

  // Accept the requested configure or any later one that includes the
  // requested size. Serial arithmetic is modular, so compare as signed.
  auto delta = static_cast<int32_t>(acked - requested);
  if (delta < 0)
    return false;
  if (acked == transition->base_configure_serial)
    return false;

  // Acknowledgement alone is not enough: the matching buffer commit must have
  // happened too. This is why the callback is a surface commit listener.
  if (base->surface->current.seq == transition->base_seq)
    return false;

  this->start_window_animation(false);
  return true;
}

void UraToplevel::start_window_animation(bool timed_out) {
  (void)timed_out;
  auto* transition =
    this->scene_model().try_get<WindowTransition>(this->entity);
  if (
    !transition || transition->phase != WindowTransitionPhase::WaitingForCommit
  )
    return;

  auto target = this->layout_target_rect();
  if (target.empty())
    target = transition->requested_target;

  transition->resolved_target = target;
  transition->phase = WindowTransitionPhase::Animating;
  this->cancel_transition_timer();

  auto* server = UraServer::get_instance();
  if (auto* animations = server->view->animations.get()) {
    animations->animate_from(
      this->entity,
      transition->from,
      target,
      transition->options
    );
  }
}

void UraToplevel::cancel_transition_timer() {
  if (this->transition_timer < 0)
    return;
  auto* server = UraServer::get_instance();
  if (server->dispatcher)
    server->dispatcher->clear_timer(this->transition_timer);
  this->transition_timer = -1;
}

void UraToplevel::schedule_transition_timeout(uint64_t generation) {
  this->cancel_transition_timer();
  auto* server = UraServer::get_instance();
  if (!server->dispatcher)
    return;

  this->transition_timer = server->dispatcher->set_timeout(
    [this, generation]() {
      this->transition_timer = -1;
      if (this->entity == entt::null)
        return;
      auto* transition =
        this->scene_model().try_get<WindowTransition>(this->entity);
      if (
        !transition
        || transition->phase != WindowTransitionPhase::WaitingForCommit
        || transition->generation != generation
      )
        return;
      // Fall back to the last valid content geometry. This moves the window
      // without stretching stale content when a client never answers.
      this->start_window_animation(true);
    },
    std::chrono::milliseconds(250)
  );
}

void UraToplevel::dismiss_popups() {
  wlr_xdg_popup *_popup, *tmp;
  wl_list_for_each_safe(_popup, tmp, &this->xdg_toplevel->base->popups, link) {
    wlr_xdg_popup_destroy(_popup);
  }
}

uint64_t UraToplevel::id() {
  return UraEntityRegistry::to_id(this->entity);
}

void UraToplevel::set_fullscreen(bool flag) {
  if (!flag) {
    this->pending_fullscreen_output.clear();
    this->restore_fullscreen_output();
  }
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
  if (this->foreign_handle)
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
}

void UraToplevel::set_fullscreen_output(UraOutput* output) {
  if (!output)
    return;
  if (!this->prepared) {
    this->pending_fullscreen_output = output->name;
    return;
  }
  if (this->output() == output)
    return;
  this->apply_fullscreen_output(output, true);
}

void UraToplevel::apply_fullscreen_output(
  UraOutput* output,
  bool save_restore
) {
  auto geometry = output->logical_geometry();
  if (geometry.width <= 0 || geometry.height <= 0)
    return;
  if (save_restore && !this->fullscreen_output_restore_valid) {
    auto target = this->layout().target;
    this->fullscreen_output_restore_geometry = Rect<int> {
      static_cast<int>(std::lround(target.x)),
      static_cast<int>(std::lround(target.y)),
      static_cast<int>(std::lround(target.width)),
      static_cast<int>(std::lround(target.height)),
    };
    auto tags = this->tags().values;
    this->fullscreen_output_restore_tags =
      Vec<std::string>(tags.begin(), tags.end());
    this->fullscreen_output_restore_valid = true;
  }
  this->move(geometry.x, geometry.y);
  auto tags = output->tags;
  this->set_tags(std::move(tags));
}

void UraToplevel::apply_pending_fullscreen_output() {
  if (this->pending_fullscreen_output.empty())
    return;
  auto server = UraServer::get_instance();
  auto output =
    server->view->get_output_by_name(this->pending_fullscreen_output);
  this->pending_fullscreen_output.clear();
  if (output)
    this->apply_fullscreen_output(output, false);
}

void UraToplevel::restore_fullscreen_output() {
  if (!this->fullscreen_output_restore_valid)
    return;
  this->fullscreen_output_restore_valid = false;
  auto geometry = this->fullscreen_output_restore_geometry;
  auto tags = std::move(this->fullscreen_output_restore_tags);
  this->move(geometry.x, geometry.y);
  this->resize(geometry.width, geometry.height);
  this->set_tags(std::move(tags));
}

bool UraToplevel::is_fullscreen() {
  return this->xdg_toplevel->current.fullscreen;
}

void UraToplevel::set_resizing(bool flag) {
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_resizing(this->xdg_toplevel, flag);
}

bool UraToplevel::is_resizing() {
  return this->xdg_toplevel->current.resizing;
}

void UraToplevel::set_maximized(bool flag) {
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_maximized(this->xdg_toplevel, flag);
}

bool UraToplevel::is_maximized() {
  return this->xdg_toplevel->current.maximized;
}

bool UraToplevel::mapped() {
  if (!this->xdg_toplevel->base->initialized)
    return false;
  return this->xdg_toplevel->base->surface->mapped
    && this->visibility().visible;
}

UraOutput* UraToplevel::output() {
  auto server = UraServer::get_instance();
  auto target = this->layout().target;
  if (target.width <= 0 || target.height <= 0)
    return server->view->current_output();

  auto x = static_cast<int>(std::lround(target.x));
  auto y = static_cast<int>(std::lround(target.y));
  auto output = wlr_output_layout_output_at(server->output_layout, x, y);
  if (!output) {
    auto center_x =
      static_cast<int>(std::lround(target.x + target.width / 2.0));
    auto center_y =
      static_cast<int>(std::lround(target.y + target.height / 2.0));
    output =
      wlr_output_layout_output_at(server->output_layout, center_x, center_y);
  }
  if (output)
    return UraOutput::from(output);
  return server->view->current_output();
}

void UraToplevel::set_output(UraOutput* output) {
  if (this->foreign_output != output) {
    if (this->foreign_output) {
      wlr_foreign_toplevel_handle_v1_output_leave(
        this->foreign_handle,
        this->foreign_output->output
      );
    }
    if (output) {
      wlr_foreign_toplevel_handle_v1_output_enter(
        this->foreign_handle,
        output->output
      );
    }
    this->foreign_output = output;
  }
  this->set_scale(output ? output->scale() : 1.0);
}

void UraToplevel::update_output() {
  this->set_output(this->output());
}

void UraToplevel::output_destroyed(UraOutput* output) {
  if (this->foreign_output == output)
    this->set_output(nullptr);
}

double UraToplevel::scale() {
  auto output = this->output();
  if (output)
    return output->scale();
  return 1.;
}

void UraToplevel::set_scale(double scale) {
  if (this->preferred_scale == scale)
    return;
  this->preferred_scale = scale;
  auto server = UraServer::get_instance();
  server->view->notify_scale(this->xdg_toplevel->base->surface, scale);
}

void UraToplevel::set_tags(Vec<std::string>&& tags) {
  auto old_tags = this->tags().values;
  WindowTags next_tags;
  next_tags.values.assign(tags.begin(), tags.end());
  this->scene_model().set_tags(this->entity, std::move(next_tags));

  auto server = UraServer::get_instance();

  if (this->is_tag_matched()) {
    this->map();
    server->seat->focus(this);
  } else {
    this->unmap();
    server->seat->focus_lru();
  }

  auto args = flexible::create_table();
  args.set("id", this->id());
  args.set(
    "from",
    sol::as_table(std::vector(old_tags.begin(), old_tags.end()))
  );
  args.set("to", sol::as_table(this->tags().values));
  server->lua->emit_hook("window-tags-change", args);
}

bool UraToplevel::is_tag_matched() {
  auto output = this->output();
  if (!output)
    return false;
  auto matched = false;
  for (auto& tag : output->tags) {
    if (this->tags().contains(tag)) {
      matched = true;
      break;
    }
  }
  return matched;
}

void UraToplevel::set_opacity(float opacity) {
  this->scene_model().set_opacity(this->entity, opacity);
}

const Effect& UraToplevel::effect() {
  return this->scene_model().get<ShaderEffect>(this->entity).effect;
}

void UraToplevel::set_effect(const Effect& effect) {
  this->scene_model().set_effect(this->entity, effect);
}

unsigned int UraToplevel::border_width() {
  return this->get_userdata<unsigned int>("border_width").value_or(1);
}

void UraToplevel::set_border_color(std::string_view color) {
  if (!util::hex2rgba(color))
    return;
  this->border_color = color;
  UraServer::get_instance()->view->schedule_frames();
}

} // namespace ura
