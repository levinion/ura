#include "ura/seat/cursor.hpp"
#include <wayland-server-protocol.h>
#include "ura/view/client.hpp"
#include "ura/core/runtime.hpp"
#include <memory>
#include "ura/core/server.hpp"
#include "ura/ura.hpp"
#include "ura/core/callback.hpp"
#include "ura/seat/seat.hpp"
#include "ura/lua/lua.hpp"

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

void UraCursor::relative_move(wlr_pointer_motion_event* event) {
  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
  wlr_cursor_move(
    this->cursor,
    &event->pointer->base,
    event->delta_x,
    event->delta_y
  );
  if (this->mode == UraCursorMode::Move)
    this->process_cursor_mode_move();
  else if (this->mode == UraCursorMode::Resize)
    this->process_cursor_mode_resize();
  else
    this->process_motion(
      event->time_msec,
      event->delta_x,
      event->delta_y,
      event->unaccel_dx,
      event->unaccel_dy
    );
}

void UraCursor::absolute_move(wlr_pointer_motion_absolute_event* event) {
  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
  double lx, ly;
  wlr_cursor_absolute_to_layout_coords(
    this->cursor,
    &event->pointer->base,
    event->x,
    event->y,
    &lx,
    &ly
  );
  if (this->mode == UraCursorMode::Move)
    this->process_cursor_mode_move();
  else if (this->mode == UraCursorMode::Resize)
    this->process_cursor_mode_resize();
  else {
    auto dx = lx - this->cursor->x;
    auto dy = ly - this->cursor->y;
    this->process_motion(event->time_msec, dx, dy, dx, dy);
  }
}

void UraCursor::set_xcursor(std::string name) {
  if (name.contains("resize"))
    return;
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

void UraCursor::process_motion(
  uint32_t time_msec,
  double dx,
  double dy,
  double dx_unaccel,
  double dy_unaccel
) {
  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto cursor_follow_mouse =
    server->lua->fetch<bool>("opt.focus_follow_mouse").value_or(true);

  // surface local coordination
  double sx, sy;
  auto client = server->foreground_client(&sx, &sy);

  // unfocus if surface is none under cursor_follow_mouse mode
  if (cursor_follow_mouse && (!client || !client.value().surface)) {
    server->seat->unfocus();
    return;
  }

  // focus surface based on its type
  auto surface = client.value().surface;
  if (surface != server->seat->seat->pointer_state.focused_surface)
    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
  if (surface != server->seat->seat->keyboard_state.focused_surface) {
    if (cursor_follow_mouse) {
      server->seat->focus(client.value());
    }
  }

  wlr_relative_pointer_manager_v1_send_relative_motion(
    server->relative_pointer_manager,
    server->seat->seat,
    static_cast<uint64_t>(time_msec) * 1000,
    dx,
    dx,
    dx_unaccel,
    dy_unaccel
  );
  wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
}

void UraCursor::process_button(wlr_pointer_button_event* event) {
  auto server = UraServer::get_instance();

  // process move window
  if (event->button == 0x110) { // left button
    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED)
      this->reset_mode();
    else {
      auto super_pressed = false;
      for (auto keyboard : server->seat->keyboards) {
        auto modifiers = keyboard->get_modifiers();
        if (modifiers & WLR_MODIFIER_LOGO) {
          super_pressed = true;
          break;
        }
      }
      if (super_pressed) {
        // begin grab
        auto toplevel = server->seat->focused_toplevel();
        if (toplevel && toplevel->draggable) {
          this->mode = UraCursorMode::Move;
          this->grab = this->position();
          this->anchor = toplevel->geometry;
        }
      }
    }
  }

  // process resize window
  if (event->button == 0x111) { // right button
    if (event->state == WL_POINTER_BUTTON_STATE_RELEASED)
      this->reset_mode();
    else {
      auto super_pressed = false;
      for (auto keyboard : server->seat->keyboards) {
        auto modifiers = keyboard->get_modifiers();
        if (modifiers & WLR_MODIFIER_LOGO) {
          super_pressed = true;
          break;
        }
      }
      if (super_pressed) {
        // begin grab
        auto toplevel = server->seat->focused_toplevel();
        if (toplevel && toplevel->draggable) {
          this->mode = UraCursorMode::Resize;
          this->grab = this->position();
          this->anchor = toplevel->geometry;
        }
      }
    }
  }

  if (this->mode != UraCursorMode::Passthrough)
    return;

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
    if ((!client || !client.value().surface)
        && server->seat->focused_client()) {
      server->seat->unfocus();
      return;
    }
    server->seat->focus(client.value());
  }
}

void UraCursor::destroy() {
  wlr_xcursor_manager_destroy(this->cursor_mgr);
  wlr_cursor_destroy(this->cursor);
}

Vec2<double> UraCursor::position() {
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
}

void UraCursor::reset_mode() {
  this->mode = UraCursorMode::Passthrough;
}

void UraCursor::process_cursor_mode_move() {
  auto server = UraServer::get_instance();
  auto toplevel = server->seat->focused_toplevel();
  if (!toplevel || !toplevel->draggable)
    this->reset_mode();
  else {
    toplevel->move(
      this->anchor.x + this->position().x - this->grab.x,
      this->anchor.y + this->position().y - this->grab.y
    );
    toplevel->request_commit();
  }
}

void UraCursor::process_cursor_mode_resize() {
  auto server = UraServer::get_instance();
  auto toplevel = server->seat->focused_toplevel();
  if (!toplevel || !toplevel->draggable)
    this->reset_mode();
  else {
    toplevel->resize(
      this->anchor.width + this->position().x - this->grab.x,
      this->anchor.height + this->position().y - this->grab.y
    );
    toplevel->request_commit();
  }
}
} // namespace ura
