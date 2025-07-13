#include "ura/server.hpp"

namespace ura {
void on_new_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto device = static_cast<wlr_input_device*>(data);
  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
      server->register_keyboard(device);
      break;
    case WLR_INPUT_DEVICE_POINTER:
      server->register_pointer(device);
      break;
    default:
      break;
  }

  // info clients with capabilities
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;

  // TODO: should find a method to store all keyboards

  // if (!wl_list_empty(&server->keyboards)) {
  caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  // }

  wlr_seat_set_capabilities(server->seat, caps);
}

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

} // namespace ura
