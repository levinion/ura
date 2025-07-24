#include "ura/cursor.hpp"
#include "ura/output.hpp"
#include "ura/runtime.hpp"
#include <memory>
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"

namespace ura {

void UraCursor::init() {
  auto server = UraServer::get_instance();
  auto cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, server->output_layout);
  auto cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
  this->cursor = cursor;
  this->cursor_mgr = cursor_mgr;
  // register callbacks
  server->runtime->register_callback(
    &this->cursor->events.motion,
    on_cursor_motion,
    nullptr
  );
  server->runtime->register_callback(
    &this->cursor->events.motion_absolute,
    on_cursor_motion_absolute,
    nullptr
  );
  server->runtime->register_callback(
    &this->cursor->events.button,
    on_cursor_button,
    nullptr
  );
  server->runtime
    ->register_callback(&this->cursor->events.axis, on_cursor_axis, nullptr);
  server->runtime
    ->register_callback(&this->cursor->events.frame, on_cursor_frame, nullptr);
  server->cursor_shape_manager =
    wlr_cursor_shape_manager_v1_create(server->display, 1);
  server->runtime->register_callback(
    &server->cursor_shape_manager->events.request_set_shape,
    on_cursor_request_set_shape,
    nullptr
  );
}

void UraCursor::relative_move(int delta_x, int delta_y) {
  wlr_cursor_move(this->cursor, this->device, delta_x, delta_y);
}

void UraCursor::absolute_move(int x, int y) {
  wlr_cursor_warp_absolute(this->cursor, this->device, x, y);
}

void UraCursor::set_xcursor(std::string name) {
  wlr_cursor_set_xcursor(this->cursor, this->cursor_mgr, name.data());
  this->xcursor_name = name;
}

void UraCursor::hide() {
  wlr_cursor_unset_image(this->cursor);
  this->visible = false;
}

void UraCursor::show() {
  wlr_cursor_set_xcursor(
    this->cursor,
    this->cursor_mgr,
    this->xcursor_name.data()
  );
  this->visible = true;
}

void UraCursor::toggle() {
  visible ? this->hide() : this->show();
}

// internal method
void UraCursor::process_motion(uint32_t time_msec) {
  auto server = UraServer::get_instance();
  double sx, sy;
  auto seat = server->seat;
  auto client = server->foreground_client(&sx, &sy);
  if (!client || !client.value().surface) {
    this->set_xcursor("left_ptr");
    return;
  }
  auto surface = client.value().surface;
  wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
  wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
  if (server->config->focus_follow_mouse
      && !server->current_output()->current_workspace->focus_stack.is_top(
        client
      )) {
    client->focus();
  }
}

void UraCursor::destroy() {
  wlr_xcursor_manager_destroy(this->cursor_mgr);
  wlr_cursor_destroy(this->cursor);
}

UraPosition<double> UraCursor::position() {
  return { this->cursor->x, this->cursor->y };
}

void UraCursor::set_theme(std::string theme, int size) {
  auto server = UraServer::get_instance();
  wlr_xcursor_manager_destroy(this->cursor_mgr);
  this->cursor_mgr =
    wlr_xcursor_manager_create(theme.empty() ? NULL : theme.data(), size);
  if (this->visible)
    this->show();
}

void UraCursor::attach_device(wlr_input_device* device) {
  wlr_cursor_attach_input_device(this->cursor, device);
  this->device = device;
}
} // namespace ura
