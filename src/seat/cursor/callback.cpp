#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/client.hpp"
#include "ura/seat/cursor.hpp"
#include "ura/seat/seat.hpp"
#include "ura/lua/lua.hpp"

namespace ura {

// callback when cursor moves
void on_cursor_motion(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_motion_event*>(data);
  server->seat->cursor->relative_move(event);
}

// callback on cursor moves absolutely
void on_cursor_motion_absolute(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_motion_absolute_event*>(data);
  server->seat->cursor->absolute_move(event);
}

// this sends the click/press event
void on_cursor_button(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_button_event*>(data);
  server->seat->cursor->process_button(event);
}

// cursor scroll event
void on_cursor_axis(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_pointer_axis_event*>(data);
  server->seat->cursor->process_axis(event);
}

void on_cursor_frame(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  wlr_seat_pointer_notify_frame(server->seat->seat);
}

// prefer using cursor_shape_v1 to set cursor, this only used to hide or show the cursor
void on_seat_request_cursor(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto event = static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
  auto focused_client = server->seat->seat->pointer_state.focused_client;
  if (focused_client == event->seat_client) {
    if (event->surface)
      server->seat->cursor->set_xcursor("left_ptr");
    else
      server->seat->cursor->hide();
  }
}

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

// TODO: impl pointer constraints protocol
void on_pointer_constraints_new_constraint(wl_listener* listener, void* data) {
  auto constraint = static_cast<wlr_pointer_constraint_v1*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &constraint->events.set_region,
    on_pointer_constraints_constraint_set_region,
    constraint
  );
  server->runtime->register_callback(
    &constraint->events.destroy,
    on_pointer_constraints_constraint_destroy,
    constraint
  );
}

void on_pointer_constraints_constraint_set_region(
  wl_listener* listener,
  void* data
) {}

void on_pointer_constraints_constraint_destroy(
  wl_listener* listener,
  void* data
) {
  auto server = UraServer::get_instance();
  auto constraint =
    server->runtime->fetch<wlr_pointer_constraint_v1*>(listener);
  server->runtime->remove(constraint);
}
} // namespace ura
