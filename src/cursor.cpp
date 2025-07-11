#include "ura/server.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"

namespace ura {

// callbacks

// callback when cursor moves
void on_cursor_motion(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, cursor_motion);
  auto event = static_cast<wlr_pointer_motion_event*>(data);
  wlr_cursor_move(
    server->cursor,
    &event->pointer->base,
    event->delta_x,
    event->delta_y
  );
  server->process_cursor_motion(event->time_msec);
}

// callback on cursor moves absolutely
void on_cursor_motion_absolute(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, cursor_motion_absolute);
  auto event = static_cast<wlr_pointer_motion_absolute_event*>(data);
  wlr_cursor_warp_absolute(
    server->cursor,
    &event->pointer->base,
    event->x,
    event->y
  );
  server->process_cursor_motion(event->time_msec);
}

// this sends the click/press event
void on_cursor_button(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, cursor_button);
  auto event = static_cast<wlr_pointer_button_event*>(data);

  // notify focused client with button pressed event
  wlr_seat_pointer_notify_button(
    server->seat,
    event->time_msec,
    event->button,
    event->state
  );

  if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
    server->reset_cursor_mode();
  } else {
    // focus client
    double sx, sy;
    wlr_surface* surface;
    auto toplevel = server->desktop_toplevel_at(&surface, &sx, &sy);
    if (toplevel != nullptr)
      toplevel->focus();
  }
}

// cursor scroll event
void on_cursor_axis(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, cursor_axis);
  auto event = static_cast<wlr_pointer_axis_event*>(data);
  wlr_seat_pointer_notify_axis(
    server->seat,
    event->time_msec,
    event->orientation,
    event->delta,
    event->delta_discrete,
    event->source,
    event->relative_direction
  );
}

void on_cursor_frame(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, cursor_frame);
  wlr_seat_pointer_notify_frame(server->seat);
}

void UraServer::register_pointer(wlr_input_device* device) {
  wlr_cursor_attach_input_device(this->cursor, device);
}

void UraServer::process_cursor_motion(uint32_t time_msec) {
  switch (this->cursor_mode) {
    case CursorMode::CURSOR_MOVE:
      this->process_cursor_move();
      break;
    case CursorMode::CURSOR_RESIZE:
      this->process_cursor_resize();
      break;
    case CursorMode::CURSOR_PASSTHROUGH:
      this->process_cursor_passthrough(time_msec);
      break;
  }
}

void UraServer::process_cursor_move() {
  auto toplevel = this->grabbed_toplevel;
  wlr_scene_node_set_position(
    &toplevel->scene_tree->node,
    this->cursor->x - this->grab_x,
    this->cursor->y - this->grab_y
  );
}

void UraServer::process_cursor_resize() {
  auto toplevel = this->grabbed_toplevel;
  double border_x = this->cursor->x - this->grab_x;
  double border_y = this->cursor->y - this->grab_y;
  int new_left = this->grab_geobox.x;
  int new_right = this->grab_geobox.x + this->grab_geobox.width;
  int new_top = this->grab_geobox.y;
  int new_bottom = this->grab_geobox.y + this->grab_geobox.height;

  if (this->resize_edges & WLR_EDGE_TOP) {
    new_top = border_y;
    if (new_top >= new_bottom) {
      new_top = new_bottom - 1;
    }
  } else if (this->resize_edges & WLR_EDGE_BOTTOM) {
    new_bottom = border_y;
    if (new_bottom <= new_top) {
      new_bottom = new_top + 1;
    }
  }
  if (this->resize_edges & WLR_EDGE_LEFT) {
    new_left = border_x;
    if (new_left >= new_right) {
      new_left = new_right - 1;
    }
  } else if (this->resize_edges & WLR_EDGE_RIGHT) {
    new_right = border_x;
    if (new_right <= new_left) {
      new_right = new_left + 1;
    }
  }

  struct wlr_box* geo_box = &toplevel->xdg_toplevel->base->geometry;
  wlr_scene_node_set_position(
    &toplevel->scene_tree->node,
    new_left - geo_box->x,
    new_top - geo_box->y
  );

  int new_width = new_right - new_left;
  int new_height = new_bottom - new_top;
  wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
}

UraToplevel*
UraServer::desktop_toplevel_at(wlr_surface** surface, double* sx, double* sy) {
  auto node = wlr_scene_node_at(
    &this->scene->tree.node,
    this->cursor->x,
    this->cursor->y,
    sx,
    sy
  );

  // check validity
  if (node == nullptr || node->type != WLR_SCENE_NODE_BUFFER) {
    return nullptr;
  }

  auto scene_buffer = wlr_scene_buffer_from_node(node);
  auto scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);

  if (!scene_surface) {
    return nullptr;
  }

  *surface = scene_surface->surface;

  auto tree = node->parent;
  while (tree != nullptr && tree->node.data == nullptr) {
    tree = tree->node.parent;
  }

  return static_cast<UraToplevel*>(tree->node.data);
}

void UraServer::process_cursor_passthrough(uint32_t time_msec) {
  double sx, sy;
  auto seat = this->seat;
  wlr_surface* surface;

  auto toplevel = this->desktop_toplevel_at(&surface, &sx, &sy);

  if (!toplevel) {
    wlr_cursor_set_xcursor(this->cursor, this->cursor_mgr, "default");
  }

  if (surface) {
    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
  } else {
    wlr_seat_pointer_clear_focus(seat);
  }
}

void UraServer::reset_cursor_mode() {
  this->cursor_mode = CursorMode::CURSOR_PASSTHROUGH;
  this->grabbed_toplevel = nullptr;
}

} // namespace ura
