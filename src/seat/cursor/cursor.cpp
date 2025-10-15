#include "ura/seat/cursor.hpp"
#include "ura/seat/pointer.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"
#include "ura/core/runtime.hpp"
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
  auto pointer = UraPointer::from(event->pointer);
  auto factor = pointer->move_speed;
  wlr_cursor_move(
    this->cursor,
    &event->pointer->base,
    event->delta_x * factor,
    event->delta_y * factor
  );
  if (this->mode == UraCursorMode::Move)
    this->process_cursor_mode_move();
  else if (this->mode == UraCursorMode::Resize)
    this->process_cursor_mode_resize();
  else
    this->process_motion(
      event->time_msec,
      event->delta_x * factor,
      event->delta_y * factor,
      event->unaccel_dx * factor,
      event->unaccel_dy * factor
    );

  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
}

void UraCursor::absolute_move(wlr_pointer_motion_absolute_event* event) {
  auto server = UraServer::get_instance();
  server->seat->notify_idle_activity();
  wlr_cursor_warp_absolute(
    this->cursor,
    &event->pointer->base,
    event->x,
    event->y
  );
  if (this->mode == UraCursorMode::Move)
    this->process_cursor_mode_move();
  else if (this->mode == UraCursorMode::Resize)
    this->process_cursor_mode_resize();
  else {
    double lx, ly;
    wlr_cursor_absolute_to_layout_coords(
      this->cursor,
      &event->pointer->base,
      event->x,
      event->y,
      &lx,
      &ly
    );
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
  auto focus_follow_mouse =
    server->lua->fetch<bool>("opt.focus_follow_mouse").value_or(true);

  // get foreground surface
  double sx, sy;
  auto client = server->view->foreground_client(&sx, &sy);
  auto surface = client ? client.value().surface : nullptr;

  // send enter if move to another surface
  if (surface != server->seat->seat->pointer_state.focused_surface) {
    if (!surface) {
      wlr_seat_pointer_notify_clear_focus(seat);
      server->seat->cursor->set_xcursor("left_ptr");
    } else
      wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
  }

  assert(surface == server->seat->seat->pointer_state.focused_surface);

  // focus if move to another surface
  if (focus_follow_mouse) {
    if (!surface) {
      server->seat->unfocus();
    } else
      server->seat->focus(client.value());
  }

  wlr_seat_pointer_notify_motion(seat, time_msec, sx, sy);
  wlr_relative_pointer_manager_v1_send_relative_motion(
    server->relative_pointer_manager,
    server->seat->seat,
    static_cast<uint64_t>(time_msec) * 1000,
    dx,
    dy,
    dx_unaccel,
    dy_unaccel
  );
}

void UraCursor::process_button(wlr_pointer_button_event* event) {
  auto server = UraServer::get_instance();
  auto process_mode = [=, this](UraCursorMode mode) {
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
          this->mode = mode;
          this->grab = this->position();
          this->anchor = toplevel->geometry;
        }
      }
    }
  };
  // process move window
  if (event->button == 0x110) { // left button
    process_mode(UraCursorMode::Move);
  }
  // process resize window
  if (event->button == 0x111) { // right button
    process_mode(UraCursorMode::Resize);
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
  auto focus_follow_mouse =
    server->lua->fetch<bool>("opt.focus_follow_mouse").value_or(true);
  // only works when focus_follow_mouse is disabled
  if (!focus_follow_mouse && event->state == WL_POINTER_BUTTON_STATE_PRESSED) {
    double sx, sy;
    auto client = server->view->foreground_client(&sx, &sy);
    // unfocus if there's no surface under cursor
    if ((!client || !client.value().surface)) {
      server->seat->unfocus();
    } else {
      // focus the client under cursor
      server->seat->focus(client.value());
    }
  }

  server->seat->notify_idle_activity();
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
    wlr_xcursor_manager_create(theme.empty() ? "default" : theme.data(), size);
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
  }
}

void UraCursor::process_axis(wlr_pointer_axis_event* event) {
  auto server = UraServer::get_instance();
  auto pointer = UraPointer::from(event->pointer);
  auto factor = pointer->scroll_speed;
  wlr_seat_pointer_notify_axis(
    server->seat->seat,
    event->time_msec,
    event->orientation,
    event->delta * factor,
    event->delta_discrete,
    event->source,
    event->relative_direction
  );
  server->seat->notify_idle_activity();
}

std::string UraCursor::get_theme() {
  return this->cursor_mgr->name ? this->cursor_mgr->name : "";
}

int UraCursor::get_size() {
  return this->cursor_mgr->size;
}
} // namespace ura
