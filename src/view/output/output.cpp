#include "flexible/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/ura.hpp"
#include "ura/util/util.hpp"
#include "ura/util/vec.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/output.hpp"
#include <array>
#include <cassert>
#include <functional>
#include "ura/view/view.hpp"
#include "ura/view/workspace.hpp"
#include "ura/lua/lua.hpp"
#include "wlr-layer-shell-unstable-v1-protocol.h"

namespace ura {

void UraOutput::init(wlr_output* _wlr_output) {
  auto server = UraServer::get_instance();

  this->output = _wlr_output;
  this->output->data = this;
  this->name = this->output->name;

  this->background = wlr_scene_rect_create(
    server->view->get_scene_tree_or_create(UraSceneLayer::Clear),
    0,
    0,
    hex2rgba("#1D727A").value().data()
  );

  auto resume = false;

  if (!server->view->indexed_workspaces.contains(this->name)) {
    this->current_workspace = this->create_workspace();
  } else {
    // output resume
    this->current_workspace = this->get_workspaces().front();
    resume = true;
  }
  this->switch_workspace(this->current_workspace);

  // bind render and allocator to this output
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

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

  // set usable area to full area
  this->usable_area = this->logical_geometry();

  this->update_background();

  // add this output to scene layout
  auto output_layout_output =
    wlr_output_layout_add_auto(server->output_layout, this->output);

  auto scene_output =
    wlr_scene_output_create(server->view->scene, this->output);
  wlr_scene_output_layout_add_output(
    server->scene_layout,
    output_layout_output,
    scene_output
  );

  server->view->outputs[name] = this;

  auto configuration = wlr_output_configuration_v1_create();
  for (auto output : server->view->outputs) {
    wlr_output_configuration_head_v1_create(configuration, this->output);
  }
  wlr_output_manager_v1_set_configuration(
    server->output_manager,
    configuration
  );

  flexible::object args;
  args.set(this->id());
  if (resume)
    server->lua->try_execute_hook("output-resume", args);
  else
    server->lua->try_execute_hook("output-new", args);

  server->globals.insert(this->id());
}

UraOutput* UraOutput::from(wlr_output* output) {
  return static_cast<UraOutput*>(output->data);
}

UraOutput* UraOutput::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id))
    return reinterpret_cast<UraOutput*>(id);
  return nullptr;
}

UraOutput* UraOutput::from(std::string_view name) {
  auto server = UraServer::get_instance();
  return server->view->get_output_by_name(name);
}

void UraOutput::set_scale(float scale) {
  if (this->output->scale != scale) {
    this->output->scale = scale;
    this->update_background();
  }
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
  server->runtime->remove(this);
  server->view->outputs.erase(this->name);
  server->globals.erase(this->id());
}

Vec<UraLayerShell*>&
UraOutput::get_layer_list_by_type(zwlr_layer_shell_v1_layer type) {
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
  if (server->view->current_output() != this)
    return false;
  auto full_area = this->logical_geometry().to_wlr_box();
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
  if (this->usable_area.x != usable_area.x
      || this->usable_area.y != usable_area.y
      || this->usable_area.width != usable_area.width
      || this->usable_area.height != usable_area.height) {
    this->usable_area = Vec4<int>::from(usable_area);
    return true;
  }
  return false;
}

Vec4<int> UraOutput::physical_geometry() {
  return { 0, 0, this->output->width, this->output->height };
}

Vec4<int> UraOutput::logical_geometry() {
  int width, height;
  wlr_output_effective_resolution(this->output, &width, &height);
  return { 0, 0, width, height };
}

Vec<UraWorkSpace*>& UraOutput::get_workspaces() {
  auto server = UraServer::get_instance();
  assert(server->view->indexed_workspaces.contains(this->name));
  return server->view->indexed_workspaces[this->name];
}

/* destroy workspace unless it:
 - is nonexistant
 - is active (current workspace)
 - has no toplevels */
void UraOutput::destroy_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return;
  if (!workspace->toplevels.empty())
    return;
  if (workspace == this->current_workspace)
    return;
  auto& workspaces = this->get_workspaces();
  workspaces.remove(workspace);
}

UraWorkSpace* UraOutput::get_workspace_at(int index) {
  auto& workspaces = this->get_workspaces();
  auto workspace = workspaces.get(index);
  return workspace ? *workspace : nullptr;
}

UraWorkSpace* UraOutput::create_workspace() {
  auto workspace = UraWorkSpace::init();
  workspace->output = this->name;
  auto server = UraServer::get_instance();
  server->view->indexed_workspaces[this->name].push_back(workspace.get());
  server->view->workspaces.push_back(std::move(workspace));
  return this->get_workspaces().back();
}

void UraOutput::switch_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return;
  if (workspace == this->current_workspace)
    return;
  this->current_workspace->disable();
  this->current_workspace = workspace;
  this->current_workspace->enable();
  auto server = UraServer::get_instance();
  server->lua->try_execute_hook("workspace-change", {});
}

void UraOutput::set_dpms_mode(bool flag) {
  wlr_output_state wlr_state {};
  this->dpms_on = flag;
  wlr_output_state_set_enabled(&wlr_state, flag);
  wlr_output_commit_state(this->output, &wlr_state);

  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, true);
  server->dispatcher->schedule_task(
    [=]() {
      wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, false);
      return true;
    },
    1000
  );
}

void UraOutput::update_background() {
  auto logical_geometry = this->logical_geometry();
  if (this->background->width != logical_geometry.width
      || this->background->height != logical_geometry.height)
    wlr_scene_rect_set_size(
      this->background,
      logical_geometry.width,
      logical_geometry.height
    );
  if (this->background->node.x != logical_geometry.x
      || this->background->node.y != logical_geometry.y)
    wlr_scene_node_set_position(
      &this->background->node,
      logical_geometry.x,
      logical_geometry.y
    );
}

uint64_t UraOutput::id() {
  return reinterpret_cast<uint64_t>(this);
}

} // namespace ura
