#include "ura/layer_shell.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include <iterator>
#include <list>
#include <utility>
#include "ura/ura.hpp"
#include "ura/layer_shell.hpp"
#include "ura/workspace.hpp"
#include "ura/runtime.hpp"

namespace ura {

UraOutput* UraOutput::get_instance(wlr_output* output) {
  auto outputs = UraServer::get_instance()->runtime->outputs;
  return *std::find_if(outputs.begin(), outputs.end(), [&](auto i) {
    return i->output == output;
  });
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

void UraOutput::create_workspace() {
  auto workspace = UraWorkSpace::init();
  workspace->output = this;
  auto p = workspace.get();
  this->workspaces.push_back(std::move(workspace));
  this->switch_workspace(p->index());
}

void UraOutput::switch_workspace(int index) {
  if (index < 0)
    return;

  // if there is no such workspace, then create one
  if (index >= this->workspaces.size()) {
    this->create_workspace();
    return;
  }

  auto it = this->workspaces.begin();
  std::advance(it, index);
  if (this->current_workspace)
    this->current_workspace->enable(false);
  this->current_workspace = it->get();
  this->current_workspace->enable(true);

  // focus toplevel
  if (!this->current_workspace->toplevels.empty())
    this->current_workspace->toplevels.front()->focus();
}

} // namespace ura
