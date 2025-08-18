#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/util/vec.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/output.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/view.hpp"
#include "ura/view/workspace.hpp"
#include "ura/lua/lua.hpp"

namespace ura {

void UraOutput::init(wlr_output* _wlr_output) {
  this->output = _wlr_output;
  this->output->data = this;
  auto server = UraServer::get_instance();
  // bind render and allocator to this output
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  auto prefered_mode = wlr_output_preferred_mode(this->output);
  this->set_mode(prefered_mode);

  // create ura output object from _wlr_output
  this->current_workspace = this->create_workspace();
  this->switch_workspace(this->current_workspace);

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

  server->view->outputs.push_back(this);

  auto configuration = wlr_output_configuration_v1_create();
  for (auto output : server->view->outputs) {
    wlr_output_configuration_head_v1_create(configuration, output->output);
  }
  wlr_output_manager_v1_set_configuration(
    server->output_manager,
    configuration
  );
}

UraOutput* UraOutput::from(wlr_output* output) {
  return static_cast<UraOutput*>(output->data);
}

void UraOutput::fresh_screen() {
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

wlr_scene_tree* UraOutput::get_layer_by_type(zwlr_layer_shell_v1_layer type) {
  wlr_scene_tree* layer;
  auto server = UraServer::get_instance();
  switch (type) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Background);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Bottom);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Overlay);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      layer = server->view->get_scene_tree_or_create(UraSceneLayer::Top);
      break;
  }
  return layer;
}

std::list<UraLayerShell*>&
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
  std::list<UraLayerShell*>& list,
  wlr_box* full_area,
  wlr_box* usable_area,
  bool exclusive
) {
  auto server = UraServer::get_instance();
  if (server->view->current_output() != this)
    return;
  // auto scene_output = wlr_scene_get_scene_output(server->scene, this->output);
  // wlr_scene_node_set_position(&layer->node, scene_output->x, scene_output->y);
  for (auto layer_shell : list) {
    if (!layer_shell->layer_surface || !layer_shell->layer_surface->initialized
        || (layer_shell->layer_surface->current.exclusive_zone > 0)
          != exclusive)
      continue;
    wlr_scene_layer_surface_v1_configure(
      layer_shell->scene_surface,
      full_area,
      usable_area
    );
  }
}

bool UraOutput::configure_layers() {
  auto full_area = this->logical_geometry().to_wlr_box();
  auto usable_area = full_area;
  for (auto exclusive : { true, false }) {
    // background
    this->configure_layer(
      this->background_surfaces,
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
    // top
    this->configure_layer(
      this->top_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // overlay
    this->configure_layer(
      this->overlay_surfaces,
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

// internal method
void UraOutput::set_mode(wlr_output_mode* mode) {
  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);
  if (mode)
    wlr_output_state_set_mode(&state, mode);
  wlr_output_commit_state(this->output, &state);
  wlr_output_state_finish(&state);
}

Vec4<int> UraOutput::physical_geometry() {
  return { 0, 0, this->output->width, this->output->height };
}

Vec4<int> UraOutput::logical_geometry() {
  int width, height;
  wlr_output_effective_resolution(this->output, &width, &height);
  return { 0, 0, width, height };
}

void UraOutput::destroy_workspace(int index) {
  if (this->workspaces.size() == 1)
    return;
  auto workspace = this->get_workspace_at(index);
  if (!workspace)
    return;
  if (!workspace->toplevels.empty())
    return;
  if (workspace == this->current_workspace)
    return;

  auto it = workspaces.begin();
  std::advance(it, index);
  this->workspaces.erase(it);
}

UraWorkSpace* UraOutput::get_workspace_at(int index) {
  if (index < 0 || index >= this->workspaces.size())
    return nullptr;
  auto it = this->workspaces.begin();
  std::advance(it, index);
  return it->get();
}

UraWorkSpace* UraOutput::create_workspace() {
  auto workspace = UraWorkSpace::init();
  workspace->output = this;
  this->workspaces.push_back(std::move(workspace));
  return this->workspaces.back().get();
}

void UraOutput::switch_workspace(int index) {
  if (index < 0 || index >= this->workspaces.size())
    return;
  auto target = this->get_workspace_at(index);
  if (target == this->current_workspace)
    return;
  this->switch_workspace(target);
}

void UraOutput::switch_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return;
  if (workspace == this->current_workspace)
    return;
  auto server = UraServer::get_instance();
  assert(workspace != server->scratchpad.get());
  this->current_workspace->disable();
  this->current_workspace = workspace;
  this->current_workspace->enable();
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

  table["index"] = this->index();
  table["scale"] = this->output->scale;
  table["refresh"] = this->output->refresh;
  table["dpms"] = this->dpms_on;

  auto workspaces = server->lua->state.create_table();
  for (auto& workspace : this->workspaces) {
    workspaces.add(workspace->to_lua_table());
  }
  table["workspaces"] = workspaces;

  return table;
}

void UraOutput::set_dpms_mode(bool flag) {
  wlr_output_state wlr_state = { 0 };
  this->dpms_on = flag;
  wlr_output_state_set_enabled(&wlr_state, flag);
  wlr_output_commit_state(this->output, &wlr_state);
}

int UraOutput::index() {
  auto server = UraServer::get_instance();
  int index = 0;
  for (auto output : server->view->outputs) {
    if (output == this)
      return index;
    index++;
  }
  std::unreachable();
}
} // namespace ura
