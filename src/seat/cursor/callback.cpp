#include <wayland-server-protocol.h>
#include "ura/callback.hpp"
#include "ura/client.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/seat.hpp"
#include "ura/lua.hpp"

namespace ura {

// callback when cursor moves
void on_cursor_motion(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_motion_event*>(data);
  server->seat->cursor->relative_move(event->delta_x, event->delta_y);
  server->seat->cursor->process_motion(event->time_msec);
}

// callback on cursor moves absolutely
void on_cursor_motion_absolute(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_motion_absolute_event*>(data);
  server->seat->cursor->absolute_move(event->x, event->y);
  server->seat->cursor->process_motion(event->time_msec);
}

// this sends the click/press event
void on_cursor_button(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_button_event*>(data);

  // notify focused client with button pressed event
  wlr_seat_pointer_notify_button(
    server->seat->seat,
    event->time_msec,
    event->button,
    event->state
  );

  // focus pressed toplevel if focus_follow_mouse is not enabled
  auto cursor_follow_mouse =
    server->lua->fetch<bool>("opt.focus_follow_mouse").value_or(true);
  if (!cursor_follow_mouse && event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    // focus client
    double sx, sy;
    auto client = server->foreground_client(&sx, &sy);
    if ((!client || !client.value().surface) && server->seat->focused()) {
      server->seat->unfocus();
      return;
    }
    if (client)
      server->seat->focus(client.value());
  }
}

// cursor scroll event
void on_cursor_axis(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_axis_event*>(data);
  wlr_seat_pointer_notify_axis(
    server->seat->seat,
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
  wlr_seat_pointer_notify_frame(server->seat->seat);
}

// prefer using cursor_shape_v1 to set cursor
// void on_seat_request_cursor(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
//
//   auto focused_client = server->seat->pointer_state.focused_client;
//   if (focused_client == event->seat_client) {
//     wlr_cursor_set_surface(
//       server->cursor,
//       event->surface,
//       event->hotspot_x,
//       event->hotspot_y
//     );
//   }
// }

void on_seat_request_set_selection(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_request_set_selection_event*>(data);
  wlr_seat_set_selection(server->seat->seat, event->source, event->serial);
}

void on_seat_request_set_primary_selection(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_request_set_primary_selection_event*>(data);
  wlr_seat_set_primary_selection(
    server->seat->seat,
    event->source,
    event->serial
  );
}

void on_seat_request_start_drag(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_request_start_drag_event*>(data);
  if (wlr_seat_validate_pointer_grab_serial(
        server->seat->seat,
        event->origin,
        event->serial
      ))
    wlr_seat_start_pointer_drag(server->seat->seat, event->drag, event->serial);
  else
    wlr_data_source_destroy(event->drag->source);
}

void on_seat_start_drag(wl_listener* listener, void* data) {}

void on_cursor_request_set_shape(wl_listener* listener, void* data) {
  auto event =
    static_cast<wlr_cursor_shape_manager_v1_request_set_shape_event*>(data);
  auto server = UraServer::get_instance();
  auto focused_client = server->seat->seat->pointer_state.focused_client;
  if (focused_client == event->seat_client) {
    auto cursor_shape_name = wlr_cursor_shape_v1_name(event->shape);
    server->seat->cursor->set_xcursor(cursor_shape_name);
  }
}
} // namespace ura
