#include "ura/toplevel.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {

// create a new toplevel
void on_new_toplevel(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, new_xdg_toplevel);
  auto xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);

  auto toplevel = new UraToplevel {};
  toplevel->xdg_toplevel = xdg_toplevel;
  toplevel->scene_tree = wlr_scene_xdg_surface_create(
    &UraServer::get_instance()->scene->tree,
    xdg_toplevel->base
  );
  toplevel->scene_tree->node.data = toplevel;
  xdg_toplevel->base->data = toplevel->scene_tree;

  // setup callback
  toplevel->map.notify = on_toplevel_map;
  wl_signal_add(&xdg_toplevel->base->surface->events.map, &toplevel->map);
  toplevel->unmap.notify = on_toplevel_unmap;
  wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &toplevel->unmap);
  toplevel->commit.notify = on_toplevel_commit;
  wl_signal_add(&xdg_toplevel->base->surface->events.commit, &toplevel->commit);
  toplevel->destroy.notify = on_toplevel_destroy;
  wl_signal_add(&xdg_toplevel->events.destroy, &toplevel->destroy);

  toplevel->request_move.notify = on_toplevel_request_move;
  wl_signal_add(&xdg_toplevel->events.request_move, &toplevel->request_move);
  toplevel->request_resize.notify = on_toplevel_request_resize;
  wl_signal_add(
    &xdg_toplevel->events.request_resize,
    &toplevel->request_resize
  );
  toplevel->request_maximize.notify = on_toplevel_request_maximize;
  wl_signal_add(
    &xdg_toplevel->events.request_maximize,
    &toplevel->request_maximize
  );
  toplevel->request_fullscreen.notify = on_toplevel_request_fullscreen;
  wl_signal_add(
    &xdg_toplevel->events.request_fullscreen,
    &toplevel->request_fullscreen
  );
}

void on_toplevel_map(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, map);
  wl_list_insert(&UraServer::get_instance()->toplevels, &toplevel->link);
  toplevel->focus();
}

void on_toplevel_unmap(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, unmap);

  auto server = UraServer::get_instance();
  // remove from toplevels
  if (toplevel == server->grabbed_toplevel) {
    server->reset_cursor_mode();
  }
  wl_list_remove(&toplevel->link);
}

void on_toplevel_commit(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, commit);
  if (toplevel->xdg_toplevel->base->initial_commit) {
    wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
  }
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, destroy);
  wl_list_remove(&toplevel->map.link);
  wl_list_remove(&toplevel->unmap.link);
  wl_list_remove(&toplevel->commit.link);
  wl_list_remove(&toplevel->destroy.link);
  wl_list_remove(&toplevel->request_move.link);
  wl_list_remove(&toplevel->request_resize.link);
  wl_list_remove(&toplevel->request_maximize.link);
  wl_list_remove(&toplevel->request_fullscreen.link);
  delete toplevel;
}

void on_toplevel_request_move(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, request_move);
  toplevel->move();
}

void on_toplevel_request_resize(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, request_resize);
  auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
  toplevel->resize(event->edges);
}

void on_toplevel_request_maximize(wl_listener* listener, void* data) {
  UraToplevel* toplevel = wl_container_of(listener, toplevel, request_maximize);
  // TODO: do nothing, but we should have some work to do
  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
}

void on_toplevel_request_fullscreen(wl_listener* listener, void* data) {
  UraToplevel* toplevel =
    wl_container_of(listener, toplevel, request_fullscreen);
  // TODO: do nothing, but we should have some work to do
  if (toplevel->xdg_toplevel->base->initialized) {
    wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
  }
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

  // move scene to top
  wlr_scene_node_raise_to_top(&this->scene_tree->node);
  wl_list_remove(&this->link);
  wl_list_insert(&server->toplevels, &this->link);

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

} // namespace ura
