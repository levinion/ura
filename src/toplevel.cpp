#include "ura/toplevel.hpp"
#include "ura/runtime.hpp"
#include "ura/output.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {

// create a new toplevel
void on_new_toplevel(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);

  // setup ura toplevel
  auto toplevel = new UraToplevel {};
  toplevel->xdg_toplevel = xdg_toplevel;
  toplevel->scene_tree = wlr_scene_xdg_surface_create(
    &UraServer::get_instance()->scene->tree,
    xdg_toplevel->base
  );
  toplevel->scene_tree->node.data = toplevel;
  xdg_toplevel->base->data = toplevel->scene_tree;

  // // notify scale
  wlr_fractional_scale_v1_notify_scale(
    xdg_toplevel->base->surface,
    server->config->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    xdg_toplevel->base->surface,
    server->config->scale
  );

  // register callback
  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.map,
    on_toplevel_map,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.unmap,
    on_toplevel_unmap,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.commit,
    on_toplevel_commit,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.destroy,
    on_toplevel_destroy,
    toplevel
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

  server->runtime->register_callback(
    &xdg_toplevel->events.request_maximize,
    on_toplevel_request_maximize,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.request_fullscreen,
    on_toplevel_request_fullscreen,
    toplevel
  );

  server->runtime->toplevels.push_back(toplevel);
}

void on_toplevel_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  if (toplevel->hidden)
    toplevel->show();
  toplevel->focus();
}

void on_toplevel_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->hide();
}

void on_toplevel_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  auto output = server->current_output();
  auto mode = output->output->current_mode;
  auto scale = server->config->scale;
  auto width = mode->width / scale;
  auto height = mode->height / scale;

  // handle fullscreen toplevel window
  if (toplevel->fullscreen()) {
    toplevel->focus();
    toplevel->resize(width, height);
    toplevel->move(0, 0);
    return;
  }

  // else auto tiling
  auto gap = 30;
  auto& toplevels = server->runtime->toplevels;
  int sum = 0;
  for (auto toplevel : toplevels) {
    if (!toplevel->hidden && !toplevel->fullscreen())
      sum += 1;
  }
  int i = 0;
  for (auto window : toplevels) {
    if (window->hidden || window->fullscreen())
      continue;
    if (window != toplevel)
      i++;
    else
      break;
  }
  toplevel->resize((width - 2 * gap) / sum, height - 2 * gap);
  toplevel->move(((width - 2 * gap) / sum * i) + gap, gap);
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  server->focused_toplevel = nullptr;
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  server->runtime->toplevels.remove(toplevel);
  server->runtime->remove(toplevel);
  delete toplevel;

  if (!server->runtime->toplevels.empty()) {
    auto new_toplevel = server->runtime->toplevels.front();
    new_toplevel->focus();
  }
}

// void on_toplevel_request_move(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   toplevel->move();
// }
//
// void on_toplevel_request_resize(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
//   toplevel->resize(event->edges);
// }

void on_toplevel_request_maximize(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  // TODO:

  // toplevel->toggle_fullscreen();
}

void on_toplevel_request_fullscreen(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->toggle_fullscreen();
}

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

} // namespace ura
