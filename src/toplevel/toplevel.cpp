#include "ura/toplevel.hpp"
#include <utility>
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"

namespace ura {

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat;
  auto prev_surface = seat->keyboard_state.focused_surface;
  auto surface = this->xdg_toplevel->base->surface;

  if (prev_surface == surface) {
    // should only focus once
    return;
  }
  if (prev_surface) {
    // unfocus previous focused toplevel
    auto prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
    if (prev_toplevel) {
      wlr_xdg_toplevel_set_activated(prev_toplevel, false);
    }
  }

  server->focused_toplevel = this;

  // move scene to top
  wlr_scene_node_raise_to_top(&this->scene_tree->node);

  // activate this toplevel
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);

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

UraToplevel* UraToplevel::from(wlr_xdg_toplevel* toplevel) {
  auto toplevels = UraServer::get_instance()->runtime->toplevels;
  return *std::find_if(toplevels.begin(), toplevels.end(), [&](auto i) {
    return i->xdg_toplevel == toplevel;
  });
}

int UraToplevel::move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto target = output->get_workspace_at(index);
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
