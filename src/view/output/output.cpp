#include "ura/view/output.hpp"
#include "ura/ext_workspace.hpp"
#include "ura/core/log.hpp"
#include "ura/render/frame_renderer.hpp"
#include "ura/scene/frame_planner.hpp"
#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/ura.hpp"
#include "ura/util/rgb.hpp"
#include "ura/util/vec.hpp"
#include "ura/util/tag.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/session_lock.hpp"
#include <array>
#include <ranges>
#include "ura/view/view.hpp"
#include "ura/core/lua.hpp"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include "ura/seat/seat.hpp"

namespace ura {

void UraOutput::init(wlr_output* _wlr_output) {
  auto server = UraServer::get_instance();

  this->output = _wlr_output;
  this->output->data = this;
  this->name = this->output->name;
  this->entity = server->entities->create_output(this);
  auto default_tags =
    server->lua->get_option<std::vector<std::string>>("default_output_tags")
      .value_or(std::vector<std::string> { ":1" });
  this->tags.clear();
  for (auto& value : default_tags) {
    if (auto tag = Tag::parse_or_default(value))
      this->tags.push_back(std::move(*tag));
  }
  if (this->tags.empty())
    this->tags.push_back(Tag { .name_space = "", .index = 1 });
  std::ranges::sort(this->tags, TagLess {});

  this->background_color = util::hex2rgba("#1D727A").value();
  wlr_damage_ring_init(&this->frame_damage_ring);

  // bind render and allocator to this output
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  if (
    wlr_backend_is_drm(_wlr_output->backend)
    && _wlr_output->backend->features.timeline
    && server->renderer->features.timeline
  ) {
    int drm_fd = wlr_renderer_get_drm_fd(server->renderer);
    if (drm_fd >= 0)
      this->render_timeline = wlr_drm_syncobj_timeline_create(drm_fd);
  }

  auto restored_context = this->restore_context();
  auto resume = restored_context.has_value();
  auto restored_geometry =
    restored_context ? restored_context->logical_geometry : Rect<int> {};
  this->last_notified_scale = this->scale();

  // register callback
  server->runtime
    ->register_callback(&this->output->events.frame, on_output_frame, this);
  server->runtime
    ->register_callback(&this->output->events.damage, on_output_damage, this);
  server->runtime->register_callback(
    &this->output->events.needs_frame,
    on_output_needs_frame,
    this
  );
  server->runtime->register_callback(
    &this->output->events.request_state,
    on_output_request_state,
    this
  );
  server->runtime
    ->register_callback(&this->output->events.destroy, on_output_destroy, this);

  if (resume && restored_geometry.width > 0 && restored_geometry.height > 0) {
    wlr_output_layout_add(
      server->output_layout,
      this->output,
      restored_geometry.x,
      restored_geometry.y
    );
  } else {
    wlr_output_layout_add_auto(server->output_layout, this->output);
  }

  // set usable area to full area
  this->usable_area = this->logical_geometry();
  this->last_logical_geometry = this->usable_area;
  this->last_geometry_enabled =
    this->usable_area.width > 0 && this->usable_area.height > 0;

  server->view->outputs[this->name] = this;
  this->attach_layer_shells();
  this->refresh_geometry();

  auto configuration = wlr_output_configuration_v1_create();
  for (auto [_, output] : server->view->outputs) {
    wlr_output_configuration_head_v1_create(configuration, output->output);
  }
  wlr_output_manager_v1_set_configuration(
    server->output_manager,
    configuration
  );

  if (server->ext_workspace) {
    server->ext_workspace->add_output(this);
    server->ext_workspace->refresh();
  }

  auto args = flexible::create_table();
  args.set("id", this->id());

  if (resume)
    server->lua->emit_hook("output-resume", args);
  else {
    server->lua->emit_hook("output-new", args);
  }
}

UraOutput* UraOutput::from(wlr_output* output) {
  return output ? static_cast<UraOutput*>(output->data) : nullptr;
}

UraOutput* UraOutput::from(uint64_t id) {
  return UraServer::get_instance()->entities->output(id);
}

UraOutput* UraOutput::from(std::string_view name) {
  auto server = UraServer::get_instance();
  return server->view->get_output_by_name(name);
}

void UraOutput::set_scale(float scale) {
  if (this->output->scale == scale)
    return;

  this->output->scale = scale;
  this->refresh_geometry();

  auto server = UraServer::get_instance();
  for (auto toplevel : server->view->toplevels) {
    if (toplevel->output() == this)
      toplevel->set_scale(scale);
  }
}

void UraOutput::refresh_geometry() {
  auto geometry = this->logical_geometry();
  bool enabled = geometry.width > 0 && geometry.height > 0;
  bool scale_changed = this->last_notified_scale != this->scale()
    || (enabled && !this->last_geometry_enabled);
  this->last_notified_scale = this->scale();
  this->last_geometry_enabled = enabled;
  if (enabled)
    this->last_logical_geometry = geometry;

  auto server = UraServer::get_instance();
  for (auto layer_shell : this->layer_shells()) {
    layer_shell->refresh_visibility(enabled);
    if (enabled && scale_changed) {
      server->view->notify_scale(
        layer_shell->layer_surface->surface,
        this->scale()
      );
    }
  }
  if (this->session_lock_surface) {
    if (enabled) {
      wlr_session_lock_surface_v1_configure(
        this->session_lock_surface->surface,
        geometry.width,
        geometry.height
      );
    }
    this->session_lock_surface->update_scene_geometry();
    this->session_lock_surface->update_visibility();
  }
  this->configure_layers();
  if (!enabled)
    server->view->frame_scheduler->invalidate_output(this);
  server->view->schedule_frames();
}

float UraOutput::scale() {
  return this->output->scale;
}

wlr_damage_ring* UraOutput::damage_ring() {
  return &this->frame_damage_ring;
}

std::array<wlr_buffer*, 2>* UraOutput::retained_buffers() {
  return &this->retained_frame_buffers;
}

void UraOutput::commit() {
  auto server = UraServer::get_instance();
  server->view->tick_animations();
  auto plan = server->view->frame_scheduler->take(this);
  plan.retained_buffers = this->retained_buffers();
  if (this->render_timeline) {
    plan.render_timeline = this->render_timeline;
    plan.render_point = ++this->render_point;
  }
  auto* profiler = server->view->frame_profiler.get();
  if (plan.logical_box.width <= 0 || plan.logical_box.height <= 0) {
    server->view->frame_scheduler->invalidate_output(this);
    profiler->record_skip();
    profiler->maybe_report();
    return;
  }
  if (!plan.full_redraw && plan.damage.empty()) {
    server->view->frame_scheduler->commit(this);
    profiler->record_skip();
    profiler->maybe_report();
    return;
  }

  auto render_start = UraFrameProfiler::Clock::now();
  bool rendered = render_frame(server->renderer, plan);
  profiler->record_render(
    UraFrameProfiler::Clock::now() - render_start,
    plan.surfaces.size(),
    plan.damage.size(),
    plan.operations.size(),
    plan.full_redraw,
    rendered
  );
  profiler->maybe_report();
  if (!rendered) {
    log::error("Ura frame renderer failed");
    server->view->frame_scheduler->abort_frame(this);
    return;
  }
  server->view->frame_scheduler->commit(this);

  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  for (auto& frame_surface : plan.surfaces) {
    auto* surface = frame_surface.surface;
    if (!surface)
      continue;
    auto* sync = wlr_linux_drm_syncobj_v1_get_surface_state(surface);
    if (
      sync && surface->buffer && surface->buffer->base.n_locks > 0
      && !wlr_linux_drm_syncobj_v1_state_signal_release_with_buffer(
        sync,
        &surface->buffer->base
      )
    )
      log::error("Failed to signal surface release timeline");
    if (frame_surface.primary_output)
      wlr_surface_send_frame_done(surface, &now);
  }
}

void UraOutput::destroy() {
  auto server = UraServer::get_instance();
  if (server->ext_workspace)
    server->ext_workspace->remove_output(this);
  for (auto toplevel : server->view->toplevels)
    toplevel->output_destroyed(this);
  this->detach_layer_shells();
  if (this->session_lock_surface) {
    auto* lock_surface = this->session_lock_surface;
    this->session_lock_surface = nullptr;
    lock_surface->output = nullptr;
    lock_surface->update_visibility();
  }
  this->save_context();
  if (this->render_timeline) {
    wlr_drm_syncobj_timeline_signal(this->render_timeline, UINT64_MAX);
    wlr_drm_syncobj_timeline_unref(this->render_timeline);
    this->render_timeline = nullptr;
  }
  for (auto*& buffer : this->retained_frame_buffers) {
    if (buffer) {
      wlr_buffer_unlock(buffer);
      buffer = nullptr;
    }
  }
  wlr_damage_ring_finish(&this->frame_damage_ring);
  server->view->frame_scheduler->remove(this);
  server->runtime->remove(this);
  server->view->outputs.erase(this->name);
  if (server->ext_workspace)
    server->ext_workspace->refresh();
  server->entities->destroy(this->entity);
  this->entity = entt::null;
}

void UraOutput::attach_layer_shells() {
  auto server = UraServer::get_instance();
  for (auto layer_shell : server->view->layer_shell_surfaces) {
    if (layer_shell->output != this->name)
      continue;
    if (layer_shell->layer_surface->initialized)
      layer_shell->layer = layer_shell->layer_surface->current.layer;
    auto& layer = this->layer_shells_from_layer(layer_shell->layer);
    if (!layer.contains(layer_shell))
      layer.push_back(layer_shell);
    layer_shell->set_scene_layer(
      server->view->get_z_index_by_type(layer_shell->layer)
    );
    layer_shell->refresh_visibility(true);
    layer_shell->layer_surface->output = this->output;
  }
}

void UraOutput::detach_layer_shells() {
  for (auto layer_shell : this->layer_shells()) {
    layer_shell->refresh_visibility(false);
    if (layer_shell->layer_surface->output == this->output)
      layer_shell->layer_surface->output = nullptr;
  }
}

Vec<UraLayerShell*>&
UraOutput::layer_shells_from_layer(zwlr_layer_shell_v1_layer type) {
  switch (type) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      return this->background_surfaces;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      return this->bottom_surfaces;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      return this->overlay_surfaces;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      return this->top_surfaces;
      break;
  }
  std::unreachable();
}

Vec<UraLayerShell*> UraOutput::layer_shells() {
  Vec<UraLayerShell*> layer_shells;
  for (auto layer : { ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
                      ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM,
                      ZWLR_LAYER_SHELL_V1_LAYER_TOP,
                      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY }) {
    auto layers = this->layer_shells_from_layer(layer);
    layer_shells.insert(layer_shells.begin(), layers.begin(), layers.end());
  }
  return layer_shells;
}

void UraOutput::configure_layer(
  Vec<UraLayerShell*>& layer_shells,
  wlr_box* full_area,
  wlr_box* usable_area,
  bool exclusive
) {
  for (auto layer_shell : layer_shells) {
    if (!layer_shell->layer_surface->initialized)
      continue;
    if ((layer_shell->layer_surface->current.exclusive_zone > 0) != exclusive) {
      continue;
    }
    layer_shell->configure(full_area, usable_area);
  }
}

bool UraOutput::configure_layers() {
  auto server = UraServer::get_instance();
  auto geometry = this->logical_geometry();
  if (geometry.width <= 0 || geometry.height <= 0) {
    for (auto layer_shell : this->layer_shells())
      layer_shell->refresh_visibility(false);
    if (this->usable_area.empty())
      return false;
    this->usable_area = {};
    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("output-usable-geometry-change", args);
    return true;
  }

  auto full_area = geometry.to_wlr_box();
  auto usable_area = full_area;
  for (auto exclusive : { true, false }) {
    // overlay
    this->configure_layer(
      this->overlay_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // top
    this->configure_layer(
      this->top_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // bottom
    this->configure_layer(
      this->bottom_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // background
    this->configure_layer(
      this->background_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
  }
  if (
    this->usable_area.x != usable_area.x || this->usable_area.y != usable_area.y
    || this->usable_area.width != usable_area.width
    || this->usable_area.height != usable_area.height
  ) {
    this->usable_area = Rect<int>::from(usable_area);
    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("output-usable-geometry-change", args);
    return true;
  }
  return false;
}

Rect<int> UraOutput::physical_geometry() {
  return { 0, 0, this->output->width, this->output->height };
}

Rect<int> UraOutput::logical_geometry() {
  auto server = UraServer::get_instance();
  wlr_box box {};
  if (!wlr_output_layout_get(server->output_layout, this->output))
    return {};
  wlr_output_layout_get_box(server->output_layout, this->output, &box);
  return { box.x, box.y, box.width, box.height };
}

void UraOutput::set_dpms_mode(bool flag) {
  if (flag) {
    if (this->output->enabled) {
      this->dpms_off = false;
      return;
    }
  } else if (!this->output->enabled) {
    this->dpms_off = true;
    return;
  }

  wlr_output_state wlr_state {};
  wlr_output_state_set_enabled(&wlr_state, flag);
  if (!wlr_output_commit_state(this->output, &wlr_state)) {
    log::error("Failed to set DPMS {} for output {}", flag, this->name);
    return;
  }
  this->dpms_off = !flag;
  auto server = UraServer::get_instance();
  // Disabling an output can make the lock/layer surface invisible on its next
  // commit. Refresh the scene state and invalidate cached plans before the
  // wake frame so the lock surface is visible again immediately.
  this->refresh_geometry();
  server->view->frame_scheduler->invalidate_output(this);
  server->view->schedule_frames();
}

uint64_t UraOutput::id() {
  return UraEntityRegistry::to_id(this->entity);
}

void UraOutput::set_tags(std::vector<Tag>&& tags) {
  auto old_tags = this->tags;
  auto next_tags = std::move(tags);
  std::ranges::sort(next_tags, TagLess {});
  this->tags = std::move(next_tags);
  auto server = UraServer::get_instance();
  if (server->ext_workspace)
    server->ext_workspace->refresh();
  for (auto toplevel : server->view->toplevels
         | std::views::filter([this](auto v) { return v->output() == this; })) {
    if (toplevel->is_tag_matched()) {
      toplevel->map();
    } else {
      toplevel->unmap();
    }
  }
  server->seat->focus_lru();

  auto args = flexible::create_table();
  args.set("id", this->id());
  auto tag_names = [](const std::vector<Tag>& values) {
    std::vector<std::string> names;
    names.reserve(values.size());
    for (auto& tag : values) names.push_back(tag.name());
    return names;
  };
  args.set("from", sol::as_table(tag_names(old_tags)));
  args.set("to", sol::as_table(tag_names(this->tags)));
  server->lua->emit_hook("output-tags-change", args);
}

void UraOutput::apply(wlr_output_configuration_v1* config) {
  size_t states_len;
  wlr_backend_output_state* states =
    wlr_output_configuration_v1_build_state(config, &states_len);
  if (!states) {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
    return;
  }
  auto server = UraServer::get_instance();
  if (wlr_backend_commit(server->backend, states, states_len)) {
    this->apply_layout(config);
    wlr_output_configuration_v1_send_succeeded(config);
    wlr_output_manager_v1_set_configuration(server->output_manager, config);
  } else {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
  }
  delete states;
}

void UraOutput::apply_layout(wlr_output_configuration_v1* config) {
  auto server = UraServer::get_instance();
  wlr_output_configuration_head_v1* head;
  wl_list_for_each(head, &config->heads, link) {
    if (!head->state.enabled) {
      wlr_output_layout_remove(server->output_layout, head->state.output);
      continue;
    }
    bool was_in_layout =
      wlr_output_layout_get(server->output_layout, head->state.output);
    auto layout_output = wlr_output_layout_add(
      server->output_layout,
      head->state.output,
      head->state.x,
      head->state.y
    );
    if (was_in_layout || !layout_output)
      continue;
  }
}

void UraOutput::save_context() {
  auto server = UraServer::get_instance();
  server->view->output_contexts[this->name] = this->context();
}

std::optional<UraOutputContext> UraOutput::restore_context() {
  auto server = UraServer::get_instance();
  auto context = server->view->output_contexts.find(this->name);
  if (context == server->view->output_contexts.end())
    return {};
  this->tags = context->second.tags;
  this->output->scale = context->second.scale;
  return context->second;
}

UraOutputContext UraOutput::context() {
  UraOutputContext ctx;
  ctx.tags = this->tags;
  ctx.logical_geometry = this->last_logical_geometry;
  ctx.scale = this->scale();
  return ctx;
}

} // namespace ura
