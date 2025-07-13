#include "ura/toplevel.hpp"
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
    &xdg_toplevel->base->surface->events.destroy,
    on_toplevel_destroy,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.request_move,
    on_toplevel_request_move,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.request_resize,
    on_toplevel_request_resize,
    toplevel
  );

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
}

void on_toplevel_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->focus();
}

void on_toplevel_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);

  // remove from toplevels
  if (toplevel == server->grabbed_toplevel) {
    server->reset_cursor_mode();
  }

  // TODO: remove toplevel from scene
}

void on_toplevel_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);

  if (toplevel->xdg_toplevel->current.fullscreen) {
    auto server = UraServer::get_instance();
    auto output = wlr_output_layout_output_at(
      server->output_layout,
      server->cursor->x,
      server->cursor->y
    );
    wlr_xdg_toplevel_set_size(
      toplevel->xdg_toplevel,
      output->current_mode->width,
      output->current_mode->height
    );
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, 0);
  } else {
    wlr_scene_node_set_position(&toplevel->scene_tree->node, 0, 0);
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 800, 600);
  }
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  // TODO: runtime should find all related objects and remove them all
  server->runtime->remove(listener);
}

void on_toplevel_request_move(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->move();
}

void on_toplevel_request_resize(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  toplevel->resize(event->edges);
}

void on_toplevel_request_maximize(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  wlr_xdg_toplevel_set_maximized(
    toplevel->xdg_toplevel,
    !toplevel->xdg_toplevel->current.maximized
  );
  wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
}

void on_toplevel_request_fullscreen(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  wlr_xdg_toplevel_set_fullscreen(
    toplevel->xdg_toplevel,
    !toplevel->xdg_toplevel->current.fullscreen
  );
  wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
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

// move window
void UraToplevel::move() {
  auto server = UraServer::get_instance();

  server->grabbed_toplevel = this;
  server->cursor_mode = CursorMode::CURSOR_MOVE;

  server->grab_x = server->cursor->x - this->scene_tree->node.x;
  server->grab_y = server->cursor->y - this->scene_tree->node.y;
}

// resize window
void UraToplevel::resize(uint32_t edges) {
  auto server = UraServer::get_instance();

  server->grabbed_toplevel = this;
  server->cursor_mode = CursorMode::CURSOR_RESIZE;

  auto geo_box = &this->xdg_toplevel->base->geometry;

  double border_x = (this->scene_tree->node.x + geo_box->x)
    + ((edges & WLR_EDGE_RIGHT) ? geo_box->width : 0);
  double border_y = (this->scene_tree->node.y + geo_box->y)
    + ((edges & WLR_EDGE_BOTTOM) ? geo_box->height : 0);
  server->grab_x = server->cursor->x - border_x;
  server->grab_y = server->cursor->y - border_y;

  server->grab_geobox = *geo_box;
  server->grab_geobox.x += this->scene_tree->node.x;
  server->grab_geobox.y += this->scene_tree->node.y;

  server->resize_edges = edges;
}

void UraToplevel::set_fullscreen(bool flag) {
  auto toplevel = this->xdg_toplevel;
  wlr_xdg_toplevel_set_fullscreen(toplevel, flag);
  wlr_xdg_surface_schedule_configure(toplevel->base);
}

} // namespace ura
