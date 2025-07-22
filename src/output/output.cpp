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

  // set usable area to full area
  this->usable_area.x = 0;
  this->usable_area.y = 0;
  wlr_output_effective_resolution(
    this->output,
    &this->usable_area.width,
    &this->usable_area.height
  );

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
  wlr_box full_area;
  full_area.x = 0;
  full_area.y = 0;
  wlr_output_effective_resolution(
    this->output,
    &full_area.width,
    &full_area.height
  );
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

void UraOutput::set_mode(wlr_output_mode* mode) {
  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);
  if (mode)
    wlr_output_state_set_mode(&state, mode);
  wlr_output_commit_state(this->output, &state);
  wlr_output_state_finish(&state);
}
} // namespace ura
