#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include <list>
#include <utility>
#include "ura/callback.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"
#include "ura/layer_shell.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto _wlr_output = static_cast<wlr_output*>(data);

  // bind render and allocator to this output
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  // enable output so it can receive commit event
  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  auto mode = wlr_output_preferred_mode(_wlr_output);
  if (mode) {
    wlr_output_state_set_mode(&state, mode);
  }

  wlr_output_commit_state(_wlr_output, &state);
  wlr_output_state_finish(&state);

  // create ura output object from _wlr_output
  auto output = new UraOutput {};
  output->output = _wlr_output;

  // create scene tree
  output->background = wlr_scene_tree_create(&server->scene->tree);
  output->bottom = wlr_scene_tree_create(&server->scene->tree);
  output->top = wlr_scene_tree_create(&server->scene->tree);
  output->overlay = wlr_scene_tree_create(&server->scene->tree);

  // register callback
  server->runtime
    ->register_callback(&_wlr_output->events.frame, on_output_frame, output);
  server->runtime->register_callback(
    &_wlr_output->events.request_state,
    on_output_request_state,
    output
  );
  server->runtime->register_callback(
    &_wlr_output->events.destroy,
    on_output_destroy,
    output
  );

  wlr_scene_node_lower_to_bottom(&output->bottom->node);
  wlr_scene_node_lower_to_bottom(&output->background->node);
  wlr_scene_node_raise_to_top(&output->top->node);
  wlr_scene_node_raise_to_top(&output->overlay->node);

  // add this output to scene layout
  auto output_layout_output =
    wlr_output_layout_add_auto(server->output_layout, _wlr_output);

  auto scene_output = wlr_scene_output_create(server->scene, _wlr_output);
  wlr_scene_output_layout_add_output(
    server->scene_layout,
    output_layout_output,
    scene_output
  );

  server->runtime->outputs.push_back(output);
}

void on_output_frame(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  output->commit_frame();
}

void on_output_request_state(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  auto event = static_cast<wlr_output_event_request_state*>(data);
  wlr_output_commit_state(output->output, event->state);
}

void on_output_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  server->runtime->remove(output);
  server->runtime->outputs.remove(output);
  delete output;
}

void UraOutput::commit_frame() {
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
  wlr_box* usable_area
) {
  auto server = UraServer::get_instance();
  auto scene_output = wlr_scene_get_scene_output(server->scene, this->output);
  wlr_scene_node_set_position(&layer->node, scene_output->x, scene_output->y);
  for (auto layer_shell : list) {
    wlr_scene_layer_surface_v1_configure(
      layer_shell->scene_surface,
      full_area,
      usable_area
    );
  }
}

void UraOutput::configure_layers() {
  wlr_box usable_area;
  wlr_output_effective_resolution(
    this->output,
    &usable_area.width,
    &usable_area.height
  );
  auto full_area = usable_area;
  this->configure_layer(
    this->background,
    this->background_surfaces,
    &full_area,
    &usable_area
  );
  this->configure_layer(
    this->bottom,
    this->bottom_surfaces,
    &full_area,
    &usable_area
  );
  this
    ->configure_layer(this->top, this->top_surfaces, &full_area, &usable_area);
  this->configure_layer(
    this->overlay,
    this->overlay_surfaces,
    &full_area,
    &usable_area
  );
}

} // namespace ura
