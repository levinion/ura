#include "ura/callback.hpp"
#include "ura/toplevel.hpp"

namespace ura {
void on_activation_request_activate(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_xdg_activation_v1_request_activate_event*>(data);
  auto surface = event->surface;
  auto toplevel = UraToplevel::from(event->surface);
  toplevel->activate();
  return;
}
} // namespace ura
