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

  // this is needed to create popup surface
  xdg_toplevel->base->data = this->scene_tree;

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
}

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat;
  auto prev_surface = seat->keyboard_state.focused_surface;
  if (!this->xdg_toplevel)
    return;
  auto surface = this->xdg_toplevel->base->surface;
  if (prev_surface) {
    // should only focus once
    if (prev_surface == surface) {
      return;
    }
    // unfocus previous focused toplevel
    auto prev_wlr_toplevel =
      wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
    if (prev_wlr_toplevel) {
      server->prev_focused_toplevel = UraToplevel::from(prev_wlr_toplevel);
      wlr_xdg_toplevel_set_activated(prev_wlr_toplevel, false);
    }
  }
  server->focused_toplevel = this;
  // move scene to top
  wlr_scene_node_raise_to_top(&this->scene_tree->node);
  // activate this toplevel
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);
  // set cursor
  // keyboard focus
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

// get toplevel instance from wlr_xdg_toplevel
UraToplevel* UraToplevel::from(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  for (auto output : server->runtime->outputs)
    for (auto& workspace : output->workspaces)
      for (auto toplevel : workspace->toplevels) {
        if (toplevel->xdg_toplevel == xdg_toplevel)
          return toplevel;
      }
  return nullptr;
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

} // namespace ura
