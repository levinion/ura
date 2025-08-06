#include "ura/session_lock.hpp"
#include "ura/server.hpp"
#include "ura/seat.hpp"

namespace ura {

void UraSessionLockSurface::focus() {
  auto server = UraServer::get_instance();
  auto keyboard = wlr_seat_get_keyboard(server->seat->seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      server->seat->seat,
      this->surface->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}

UraSessionLockSurface* UraSessionLockSurface::from(wlr_surface* surface) {
  return static_cast<UraSessionLockSurface*>(surface->data);
}

} // namespace ura
