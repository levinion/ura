#include "ura/view/output.hpp"
#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/ura.hpp"
#include "ura/util/rgb.hpp"
#include "ura/util/vec.hpp"
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
  auto tags =
    server->lua->get_option<std::vector<std::string>>("default_output_tags")
      .value_or(std::vector<std::string> { ":1" });
  this->tags = Vec<std::string>(tags.begin(), tags.end());

  this->background = wlr_scene_rect_create(
    server->view->get_scene_tree_or_create(UraSceneLayer::Clear),
    0,
    0,
    util::hex2rgba("#1D727A").value().data()
  );

  // bind render and allocator to this output
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  auto restored_context = this->restore_context();
  auto resume = restored_context.has_value();
  auto restored_geometry =
    restored_context ? restored_context->logical_geometry : Vec4<int> {};
  this->last_notified_scale = this->scale();

  // register callback
  server->runtime
    ->register_callback(&this->output->events.frame, on_output_frame, this);
  server->runtime->register_callback(
    &this->output->events.request_state,
    on_output_request_state,
    this
  );
  server->runtime
    ->register_callback(&this->output->events.destroy, on_output_destroy, this);

  // add this output to scene layout before using its global logical geometry
  wlr_output_layout_output* output_layout_output;
  if (resume && restored_geometry.width > 0 && restored_geometry.height > 0) {
    output_layout_output = wlr_output_layout_add(
      server->output_layout,
      this->output,
      restored_geometry.x,
      restored_geometry.y
    );
  } else {
    output_layout_output =
      wlr_output_layout_add_auto(server->output_layout, this->output);
  }

  auto scene_output =
    wlr_scene_output_create(server->view->scene, this->output);
  wlr_scene_output_layout_add_output(
    server->scene_layout,
    output_layout_output,
    scene_output
  );

  // set usable area to full area
  this->usable_area = this->logical_geometry();
  this->last_logical_geometry = this->usable_area;
  this->last_geometry_enabled =
    this->usable_area.width > 0 && this->usable_area.height > 0;
  this->update_background();

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

  auto args = flexible::create_table();
  args.set("id", this->id());

  if (resume)
    server->lua->emit_hook("output-resume", args);
  else {
    server->lua->emit_hook("output-new", args);
  }

  server->globals[this->id()] = UraGlobalType::Output;
}

UraOutput* UraOutput::from(wlr_output* output) {
  return output ? static_cast<UraOutput*>(output->data) : nullptr;
}

UraOutput* UraOutput::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (
    server->globals.contains(id)
    && server->globals[id].type == UraGlobalType::Output
  )
    return reinterpret_cast<UraOutput*>(id);
  return nullptr;
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
  this->update_background();

  auto server = UraServer::get_instance();
  for (auto layer_shell : this->layer_shells()) {
    wlr_scene_node_set_enabled(
      &layer_shell->scene_tree->node,
      enabled && layer_shell->layer_surface->surface->mapped
    );
    if (enabled && scale_changed) {
      server->view->notify_scale(
        layer_shell->layer_surface->surface,
        this->scale()
      );
    }
  }
  if (this->session_lock_surface) {
    if (enabled) {
      wlr_scene_node_set_enabled(
        &this->session_lock_surface->scene_tree->node,
        true
      );
      wlr_scene_node_set_position(
        &this->session_lock_surface->scene_tree->node,
        geometry.x,
        geometry.y
      );
      wlr_session_lock_surface_v1_configure(
        this->session_lock_surface->surface,
        geometry.width,
        geometry.height
      );
    } else {
      wlr_scene_node_set_enabled(
        &this->session_lock_surface->scene_tree->node,
        false
      );
    }
  }
  this->configure_layers();
}

float UraOutput::scale() {
  return this->output->scale;
}

void UraOutput::commit() {
  auto server = UraServer::get_instance();
  auto scene = server->view->scene;
  auto scene_output = wlr_scene_get_scene_output(scene, this->output);
  // commit scene_output
  wlr_scene_output_commit(scene_output, nullptr);
  // notify clients
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}

void UraOutput::destroy() {
  auto server = UraServer::get_instance();
  for (auto toplevel : server->view->toplevels)
    toplevel->output_destroyed(this);
  this->detach_layer_shells();
  if (this->session_lock_surface) {
    wlr_scene_node_set_enabled(
      &this->session_lock_surface->scene_tree->node,
      false
    );
    this->session_lock_surface->output = nullptr;
    this->session_lock_surface = nullptr;
  }
  this->save_context();
  server->runtime->remove(this);
  server->view->outputs.erase(this->name);
  server->globals.erase(this->id());
  wlr_scene_node_destroy(&this->background->node);
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
    auto scene_layer = server->view->get_scene_tree_or_create(
      server->view->get_z_index_by_type(layer_shell->layer)
    );
    wlr_scene_node_reparent(&layer_shell->scene_tree->node, scene_layer);
    layer_shell->layer_surface->output = this->output;
  }
}

void UraOutput::detach_layer_shells() {
  for (auto layer_shell : this->layer_shells()) {
    wlr_scene_node_set_enabled(&layer_shell->scene_tree->node, false);
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
    wlr_scene_layer_surface_v1_configure(
      layer_shell->scene_surface,
      full_area,
      usable_area
    );
  }
}

bool UraOutput::configure_layers() {
  auto server = UraServer::get_instance();
  auto geometry = this->logical_geometry();
  if (geometry.width <= 0 || geometry.height <= 0) {
    for (auto layer_shell : this->layer_shells())
      wlr_scene_node_set_enabled(&layer_shell->scene_tree->node, false);
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
    this->usable_area = Vec4<int>::from(usable_area);
    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("output-usable-geometry-change", args);
    return true;
  }
  return false;
}

Vec4<int> UraOutput::physical_geometry() {
  return { 0, 0, this->output->width, this->output->height };
}

Vec4<int> UraOutput::logical_geometry() {
  auto server = UraServer::get_instance();
  wlr_box box {};
  if (!wlr_output_layout_get(server->output_layout, this->output))
    return {};
  wlr_output_layout_get_box(server->output_layout, this->output, &box);
  return { box.x, box.y, box.width, box.height };
}

void UraOutput::set_dpms_mode(bool flag) {
  wlr_output_state wlr_state {};
  wlr_output_state_set_enabled(&wlr_state, flag);
  wlr_output_commit_state(this->output, &wlr_state);
}

void UraOutput::update_background() {
  auto logical_geometry = this->logical_geometry();
  wlr_scene_node_set_enabled(
    &this->background->node,
    logical_geometry.width > 0 && logical_geometry.height > 0
  );
  if (logical_geometry.width > 0 && logical_geometry.height > 0
      && (this->background->width != logical_geometry.width
          || this->background->height != logical_geometry.height))
    wlr_scene_rect_set_size(
      this->background,
      logical_geometry.width,
      logical_geometry.height
    );
  if (
    this->background->node.x != logical_geometry.x
    || this->background->node.y != logical_geometry.y
  )
    wlr_scene_node_set_position(
      &this->background->node,
      logical_geometry.x,
      logical_geometry.y
    );
}

uint64_t UraOutput::id() {
  return reinterpret_cast<uint64_t>(this);
}

void UraOutput::set_tags(Vec<std::string>&& tags) {
  auto old_tags = this->tags;
  this->tags = tags;
  auto server = UraServer::get_instance();
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
  args.set(
    "from",
    sol::as_table(std::vector(old_tags.begin(), old_tags.end()))
  );
  args.set(
    "to",
    sol::as_table(std::vector(this->tags.begin(), this->tags.end()))
  );
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

    auto scene_output =
      wlr_scene_get_scene_output(server->view->scene, head->state.output);
    if (!scene_output) {
      scene_output =
        wlr_scene_output_create(server->view->scene, head->state.output);
    }
    if (scene_output) {
      wlr_scene_output_layout_add_output(
        server->scene_layout,
        layout_output,
        scene_output
      );
    }
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
