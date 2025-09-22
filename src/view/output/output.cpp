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
#include "ura/core/log.hpp"
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

  if (!this->try_set_custom_mode())
    this->set_preferred_mode();

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

  if (resume)
    server->lua->try_execute_hook("output-resume", this->name);
  else
    server->lua->try_execute_hook("output-new", this->name);
}

UraOutput* UraOutput::from(wlr_output* output) {
  return static_cast<UraOutput*>(output->data);
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
    this->current_workspace->redraw();
    return true;
  }
  return false;
}

wlr_output_mode*
UraOutput::find_nearest_mode(int width, int height, int refresh) {
  wlr_output_mode* nearest_mode = nullptr;
  auto min_diff = -1;
  wlr_output_mode* mode = nullptr;
  wl_list_for_each(mode, &this->output->modes, link) {
    if (mode->width == width && mode->height == height) {
      auto diff = std::abs(mode->refresh - refresh);
      if (min_diff < 0 || diff < min_diff) {
        min_diff = diff;
        nearest_mode = mode;
      }
    }
  }
  return nearest_mode;
}

bool UraOutput::set_mode(wlr_output_mode mode) {
  auto nearest_mode =
    this->find_nearest_mode(mode.width, mode.height, mode.refresh);
  if (!nearest_mode)
    return false;
  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);
  wlr_output_state_set_mode(&state, nearest_mode);
  if (wlr_output_commit_state(this->output, &state)) {
    this->mode = *nearest_mode;
    wlr_output_state_finish(&state);
    this->configure_layers();
    return true;
  } else {
    wlr_output_state_finish(&state);
  }
  return false;
}

bool UraOutput::set_mode(sol::table& mode) {
  auto server = UraServer::get_instance();
  auto _mode =
    this->mode ? this->mode.value() : *wlr_output_preferred_mode(this->output);
  auto height = server->lua->fetch<int>(mode, "height");
  if (height && height.value() != _mode.height) {
    _mode.height = height.value();
    _mode.preferred = false;
  }
  auto width = server->lua->fetch<int>(mode, "width");
  if (width && width.value() != _mode.width) {
    _mode.width = width.value();
    _mode.preferred = false;
  }
  auto refresh = server->lua->fetch<float>(mode, "refresh");
  if (refresh && refresh.value() != _mode.refresh) {
    _mode.refresh = refresh.value() * 1000;
    _mode.preferred = false;
  }

  auto result = this->set_mode(_mode);
  if (result) {
    // apply scale only if the properties are successfully set.
    auto scale = server->lua->fetch<float>(mode, "scale");
    if (scale && scale.value() != this->output->scale) {
      this->set_scale(scale.value());
    }
    return true;
  }
  log::warn(
    "failed to set custom mode as {}, fallback to preferred mode",
    std::format(
      "{}:{}@{}Hz",
      _mode.width,
      _mode.height,
      static_cast<float>(_mode.refresh) / 1000.f
    )
  );
  return false;
}

bool UraOutput::try_set_custom_mode() {
  auto server = UraServer::get_instance();
  auto output_table = server->lua->fetch<sol::table>("opt.device.outputs");
  if (output_table) {
    auto custom_mode =
      output_table.value()[this->name].get<std::optional<sol::table>>();
    if (custom_mode) {
      return this->set_mode(custom_mode.value());
    }
  }
  return false;
}

bool UraOutput::set_preferred_mode() {
  auto mode = wlr_output_preferred_mode(this->output);
  return this->set_mode(*mode);
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
void UraOutput::destroy_workspace(int index) {
  auto& workspaces = this->get_workspaces();
  if (workspaces.size() == 1)
    return;
  auto workspace = this->get_workspace_at(index);
  if (!workspace)
    return;
  if (!workspace->toplevels.empty())
    return;
  if (workspace == this->current_workspace)
    return;

  workspaces.remove_n(index);
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

void UraOutput::switch_workspace(int index) {
  auto target = this->get_workspace_at(index);
  if (!target || target == this->current_workspace)
    return;
  this->switch_workspace(target);
}

void UraOutput::switch_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return;
  if (workspace == this->current_workspace)
    return;
  auto server = UraServer::get_instance();
  this->current_workspace->disable();
  this->current_workspace = workspace;
  this->current_workspace->enable();
  this->current_workspace->redraw();
  server->lua->try_execute_hook("workspace-change");
}

sol::table UraOutput::to_lua_table() {
  auto server = UraServer::get_instance();
  auto table = server->lua->state.create_table();

  auto logical_geometry = this->logical_geometry();
  auto size = server->lua->state.create_table();
  size["x"] = logical_geometry.x;
  size["y"] = logical_geometry.y;
  size["width"] = logical_geometry.width;
  size["height"] = logical_geometry.height;
  table["size"] = size;

  auto usable = server->lua->state.create_table();
  usable["x"] = this->usable_area.x;
  usable["y"] = this->usable_area.y;
  usable["width"] = this->usable_area.width;
  usable["height"] = this->usable_area.height;
  table["usable"] = usable;

  table["name"] = this->name;
  table["scale"] = this->output->scale;
  table["refresh"] = this->output->refresh;
  table["dpms"] = this->dpms_on;

  auto workspaces = server->lua->state.create_table();
  for (auto workspace : this->get_workspaces()) {
    workspaces.add(workspace->to_lua_table());
  }
  table["workspaces"] = workspaces;

  return table;
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

} // namespace ura
