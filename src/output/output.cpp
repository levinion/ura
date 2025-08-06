#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include <list>
#include <utility>
#include "ura/ura.hpp"
#include "ura/layer_shell.hpp"
#include "ura/workspace.hpp"
#include "ura/runtime.hpp"
#include "ura/callback.hpp"
#include "ura/lua.hpp"

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

  // create scene tree
  this->background = wlr_scene_tree_create(&server->scene->tree);
  this->bottom = wlr_scene_tree_create(&server->scene->tree);
  this->normal = wlr_scene_tree_create(&server->scene->tree);
  this->floating = wlr_scene_tree_create(&server->scene->tree);
  this->top = wlr_scene_tree_create(&server->scene->tree);
  this->popup = wlr_scene_tree_create(&server->scene->tree);
  this->fullscreen = wlr_scene_tree_create(&server->scene->tree);
  this->overlay = wlr_scene_tree_create(&server->scene->tree);
  this->lock_screen = wlr_scene_tree_create(&server->scene->tree);

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

  // order of layers
  wlr_scene_node_raise_to_top(&this->background->node);
  wlr_scene_node_raise_to_top(&this->bottom->node);
  wlr_scene_node_raise_to_top(&this->normal->node);
  wlr_scene_node_raise_to_top(&this->floating->node);
  wlr_scene_node_raise_to_top(&this->top->node);
  wlr_scene_node_raise_to_top(&this->popup->node);
  wlr_scene_node_raise_to_top(&this->fullscreen->node);
  wlr_scene_node_raise_to_top(&this->overlay->node);
  wlr_scene_node_raise_to_top(&this->lock_screen->node);

  // set usable area to full area
  this->usable_area = this->logical_geometry();

  // add this output to scene layout
  auto output_layout_output =
    wlr_output_layout_add_auto(server->output_layout, this->output);

  auto scene_output = wlr_scene_output_create(server->scene, this->output);
  wlr_scene_output_layout_add_output(
    server->scene_layout,
    output_layout_output,
    scene_output
  );

  server->runtime->outputs.push_back(this);
  server->update_output_configuration();
}

UraOutput* UraOutput::from(wlr_output* output) {
  return static_cast<UraOutput*>(output->data);
}

void UraOutput::fresh_screen() {
  auto server = UraServer::get_instance();
  auto scene = server->scene;
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
  switch (type) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      layer = this->background;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      layer = this->bottom;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      layer = this->overlay;
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      layer = this->top;
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
  wlr_scene_tree* layer,
  std::list<UraLayerShell*>& list,
  wlr_box* full_area,
  wlr_box* usable_area,
  bool exclusive
) {
  auto server = UraServer::get_instance();
  if (server->current_output() != this)
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

void UraOutput::configure_layers() {
  auto full_area = this->logical_geometry();
  auto usable_area = full_area;
  for (auto exclusive : { true, false }) {
    // background
    this->configure_layer(
      this->background,
      this->background_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // bottom
    this->configure_layer(
      this->bottom,
      this->bottom_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // top
    this->configure_layer(
      this->top,
      this->top_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
    // overlay
    this->configure_layer(
      this->overlay,
      this->overlay_surfaces,
      &full_area,
      &usable_area,
      exclusive
    );
  }
  this->usable_area = usable_area;
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

wlr_box UraOutput::physical_geometry() {
  return { 0, 0, this->output->width, this->output->height };
}

wlr_box UraOutput::logical_geometry() {
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

bool UraOutput::switch_workspace(int index) {
  if (index < 0)
    return false;
  auto target = this->get_workspace_at(index);
  if (target == this->current_workspace)
    return true;
  return this->switch_workspace(target);
}

bool UraOutput::switch_workspace(UraWorkSpace* workspace) {
  if (!workspace)
    return false;
  if (workspace == this->current_workspace)
    return true;
  auto server = UraServer::get_instance();
  assert(workspace != server->scratchpad.get());
  this->current_workspace->disable();
  this->current_workspace = workspace;
  this->current_workspace->enable();
  server->lua->try_execute_hook("workspace-change");
  return true;
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

  table["scale"] = this->output->scale;
  table["refresh"] = this->output->refresh;
  return table;
}
} // namespace ura
