#include "ura/toplevel.hpp"
#include <utility>
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

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat;
  auto surface = this->xdg_toplevel->base->surface;
  if (server->focused_toplevel) {
    server->prev_focused_toplevel = server->focused_toplevel;
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
    wlr_xdg_toplevel_set_activated(
      server->focused_toplevel->xdg_toplevel,
      false
    );
  }
  server->focused_toplevel = this;
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
  target->toplevels.push_back(this);
  this->workspace = target;
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
} // namespace ura
