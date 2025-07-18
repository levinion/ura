#include <wayland-server-protocol.h>
#include "ura/server.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"

namespace ura {

// callback when cursor moves
void on_cursor_motion(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
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
  auto server = UraServer::get_instance();
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
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_button_event*>(data);

  // notify focused client with button pressed event
  wlr_seat_pointer_notify_button(
    server->seat,
    event->time_msec,
    event->button,
    event->state
  );

  // focus pressed toplevel if focus_follow_mouse is not enabled
  if (!server->config->focus_follow_mouse
      && event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    // focus client
    double sx, sy;
    wlr_surface* surface = nullptr;
    auto toplevel = server->foreground_toplevel(&surface, &sx, &sy);
    if (toplevel != nullptr)
      toplevel->focus();
  }
}

// cursor scroll event
void on_cursor_axis(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
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
  auto server = UraServer::get_instance();
  wlr_seat_pointer_notify_frame(server->seat);
}

void UraServer::process_cursor_motion(uint32_t time_msec) {
  double sx, sy;
  wlr_surface* surface = nullptr;
  auto toplevel = this->foreground_toplevel(&surface, &sx, &sy);

  // applications will set cursor by themselves, so we only need to set xcursor when there's no focused toplevel
  if (!toplevel) {
    wlr_cursor_set_xcursor(this->cursor, this->cursor_mgr, "default");
  }

  auto seat = this->seat;
  if (surface) {
    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
    if (this->config->focus_follow_mouse)
      toplevel->focus();
  }
}

// prefer using cursor_shape_v1 to set cursor
void on_seat_request_cursor(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);

  auto focused_client = server->seat->pointer_state.focused_client;
  if (focused_client == event->seat_client) {
    wlr_cursor_set_surface(
      server->cursor,
      event->surface,
      event->hotspot_x,
      event->hotspot_y
    );
  }
}

void on_seat_request_set_selection(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_request_set_selection_event*>(data);

  auto focused_client = server->seat->pointer_state.focused_client;
  wlr_seat_set_selection(server->seat, event->source, event->serial);
}

void on_cursor_request_set_shape(wl_listener* listener, void* data) {
  auto event =
    static_cast<wlr_cursor_shape_manager_v1_request_set_shape_event*>(data);

  auto server = UraServer::get_instance();
  auto focused_client = server->seat->pointer_state.focused_client;
  if (focused_client == event->seat_client) {
    auto cursor_shape_name = wlr_cursor_shape_v1_name(event->shape);
    wlr_cursor_set_xcursor(
      server->cursor,
      server->cursor_mgr,
      cursor_shape_name
    );
  }
}
} // namespace ura
