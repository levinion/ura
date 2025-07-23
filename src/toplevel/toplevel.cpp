#include "ura/toplevel.hpp"
#include <utility>
#include "ura/client.hpp"
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  // setup ura toplevel
  this->xdg_toplevel = xdg_toplevel;
  // add to output's normal layer
  this->scene_tree =
    wlr_scene_xdg_surface_create(output->normal, xdg_toplevel->base);
  this->output = output;
  this->workspace = this->output->current_workspace;
  this->workspace->toplevels.push_back(this);
  this->workspace->focus_stack.push(this);
  xdg_toplevel->base->surface->data = this;

  // // notify scale
  wlr_fractional_scale_v1_notify_scale(
    xdg_toplevel->base->surface,
    output->output->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    xdg_toplevel->base->surface,
    output->output->scale
  );

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
    //   &xdg_toplevel->events.request_move,
    //   on_toplevel_request_move,
    //   toplevel
    // );
    //
    // server->runtime->register_callback(
    //   &xdg_toplevel->events.request_resize,
    //   on_toplevel_request_resize,
    //   toplevel
    // );
    // server->runtime->register_callback(
    //   &xdg_toplevel->events.request_maximize,
    //   on_toplevel_request_maximize,
    //   this
    // );
    server->runtime->register_callback(
      &xdg_toplevel->events.request_fullscreen,
      on_toplevel_request_fullscreen,
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
  wlr_foreign_toplevel_handle_v1_output_enter(
    this->foreign_handle,
    this->output->output
  );
}

void UraToplevel::destroy() {
  auto server = UraServer::get_instance();
  auto workspace = this->workspace;
  auto is_top = workspace->focus_stack.is_top(this);
  workspace->focus_stack.remove(this);
  auto top = workspace->focus_stack.top();
  if (is_top && top) {
    top.value().focus();
  }
  server->runtime->remove(this);
  workspace->toplevels.remove(this);
}

void UraToplevel::commit() {
  auto server = UraServer::get_instance();
  if (!this->mapped || !this->xdg_toplevel->base->initialized) {
    return;
  }
  auto mode = this->output->logical_geometry();

  // handle fullscreen toplevel window
  if (this->fullscreen()) {
    this->resize(mode.width, mode.height);
    this->move(mode.x, mode.y);
    return;
  }

  if (this->xdg_toplevel->base->initial_commit && this->decoration) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      this->decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }

  auto usable_area = output->usable_area;
  auto width = usable_area.width;
  auto height = usable_area.height;

  auto outer_l = server->config->outer_gap_left;
  auto outer_r = server->config->outer_gap_right;
  auto outer_t = server->config->outer_gap_top;
  auto outer_b = server->config->outer_gap_bottom;
  auto inner = server->config->inner_gap;
  auto& toplevels = output->current_workspace->toplevels;
  // find mapped toplevel number
  int sum = 0;
  for (auto toplevel : toplevels) {
    if (toplevel->is_normal())
      sum += 1;
  }
  // find this toplevel index
  int i = 0;
  for (auto window : toplevels) {
    if (!window->is_normal())
      continue;
    if (window != this)
      i++;
    else
      break;
  }
  auto gaps = sum - 1;
  auto w = (width - (outer_r + outer_l) - inner * gaps) / sum;
  auto h = height - (outer_t + outer_b);
  auto x = usable_area.x + outer_l + (w + inner) * i;
  auto y = usable_area.y + outer_t;
  // check value
  if (w < 0 || h < 0 || x + w > width || y + h > height)
    return;
  this->resize(w, h);
  this->move(x, y);
}

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat;
  auto surface = this->xdg_toplevel->base->surface;
  auto workspace = this->workspace;
  // if not on top of focus stack, then unfocus the current top
  if (!workspace->focus_stack.is_top(this)) {
    auto prev = workspace->focus_stack.top().value();
    if (prev.type == UraSurfaceType::Toplevel) {
      wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
      wlr_xdg_toplevel_set_activated(
        prev.transform<UraToplevel>()->xdg_toplevel,
        false
      );
    }
  }
  // move to top of stack and focus this
  workspace->focus_stack.move_to_top(this);
  if (!this->mapped)
    this->map();
  wlr_scene_node_raise_to_top(&this->scene_tree->node);
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
}

// get toplevel instance from wlr_surface
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

int UraToplevel::move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto target = output->get_workspace_at(index);
  if (!target)
    return -1;
  this->workspace->toplevels.remove(this);
  this->workspace->focus_stack.remove(this);
  target->toplevels.push_back(this);
  this->workspace = target;
  this->workspace->focus_stack.push(this);
  return this->workspace->index();
}

int UraToplevel::index() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  int index = 0;
  for (auto toplevel : output->current_workspace->toplevels) {
    if (toplevel == this)
      return index;
    index++;
  }
  std::unreachable();
}

void UraToplevel::activate() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  this->move_to_workspace(output->current_workspace->index());
  this->focus();
  output->fresh_screen();
}

void UraToplevel::move(int x, int y) {
  wlr_scene_node_set_position(&this->scene_tree->node, x, y);
}

void UraToplevel::resize(int width, int height) {
  wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
}

void UraToplevel::set_fullscreen(bool flag) {
  if (this->xdg_toplevel->base->initialized) {
    wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
  }
}

bool UraToplevel::fullscreen() {
  if (!this->xdg_toplevel)
    return false;
  return this->xdg_toplevel->pending.fullscreen;
}

void UraToplevel::toggle_fullscreen() {
  this->set_fullscreen(!this->fullscreen());
}

void UraToplevel::close() {
  wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  wlr_foreign_toplevel_handle_v1_output_leave(
    this->foreign_handle,
    this->output->output
  );
}

void UraToplevel::map() {
  this->mapped = true;
  wlr_scene_node_set_enabled(&this->scene_tree->node, true);
}

void UraToplevel::unmap() {
  this->mapped = false;
  wlr_scene_node_set_enabled(&this->scene_tree->node, false);
}

std::string UraToplevel::title() {
  return this->xdg_toplevel->title;
}

void UraToplevel::set_title(std::string title) {
  this->xdg_toplevel->title = title.data();
  wlr_foreign_toplevel_handle_v1_set_title(this->foreign_handle, title.data());
}

bool UraToplevel::is_normal() {
  return (this->mapped && !this->fullscreen() && !this->floating);
}

wlr_box UraToplevel::logical_geometry() {
  int lx, ly;
  wlr_scene_node_coords(&this->scene_tree->node, &lx, &ly);
  return { lx,
           ly,
           this->xdg_toplevel->current.width,
           this->xdg_toplevel->current.height };
}
} // namespace ura
