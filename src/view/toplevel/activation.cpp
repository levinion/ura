#include "ura/callback.hpp"
#include "ura/client.hpp"
#include "ura/toplevel.hpp"

namespace ura {
void on_activation_request_activate(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_xdg_activation_v1_request_activate_event*>(data);
  auto surface = event->surface;
  auto client = UraClient::from(surface);
  if (client.type == UraSurfaceType::Toplevel) {
    auto toplevel = client.transform<UraToplevel>();
    toplevel->activate();
  }
}
} // namespace ura
