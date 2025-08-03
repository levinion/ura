#include "ura/seat.hpp"
#include <memory>
#include "ura/client.hpp"
#include "ura/server.hpp"
#include "ura/runtime.hpp"
#include "ura/callback.hpp"
#include "ura/toplevel.hpp"
#include "ura/ura.hpp"
#include "ura/text_input.hpp"

namespace ura {
void UraSeat::init() {
  auto server = UraServer::get_instance();
  this->seat = wlr_seat_create(server->display, "seat0");
  server->runtime->register_callback(
    &server->backend->events.new_input,
    on_new_input,
    nullptr
  );
  // this->runtime->register_callback(
  //   &this->seat->events.request_set_cursor,
  //   on_seat_request_cursor,
  //   nullptr
  // );
  server->runtime->register_callback(
    &this->seat->events.request_set_selection,
    on_seat_request_set_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_primary_selection,
    on_seat_request_set_primary_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_start_drag,
    on_seat_request_start_drag,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.start_drag,
    on_seat_start_drag,
    nullptr
  );
  this->cursor = std::make_unique<UraCursor>();
  this->cursor->init();
  this->text_input = std::make_unique<UraTextInput>();
}

void UraSeat::unfocus() {
  auto server = UraServer::get_instance();
  if (this->focused) {
    if (!this->focused->destroying)
      this->focused->unfocus();
    this->focused = nullptr;
    wlr_seat_keyboard_notify_clear_focus(seat);
    wlr_seat_pointer_notify_clear_focus(seat);
    this->cursor->set_xcursor("left_ptr");
    server->lua->try_execute_hook("focus-change");
  }
}

void UraSeat::focus(UraClient client) {
  auto server = UraServer::get_instance();
  if (!client.surface
      || this->seat->keyboard_state.focused_surface == client.surface)
    return;
  if (client.type == UraSurfaceType::Toplevel) {
    if (this->focused)
      this->focused->unfocus();
    this->focused = client.transform<UraToplevel>();
  }
  client.focus();
  server->lua->try_execute_hook("focus-change");
}

void UraSeat::focus(UraToplevel* toplevel) {
  this->focus(UraClient::from(toplevel));
}

void UraSeat::focus(UraLayerShell* layer_shell) {
  this->focus(UraClient::from(layer_shell));
}
} // namespace ura
