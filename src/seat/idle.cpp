#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/seat/seat.hpp"

namespace ura {

void on_new_idle_inhibitor(wl_listener* listener, void* data) {
  auto inhibitor = static_cast<wlr_idle_inhibitor_v1*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &inhibitor->events.destroy,
    on_idle_inhibitor_destroy,
    inhibitor
  );
  server->seat->set_idle_inhibitor(true);
}

void on_idle_inhibitor_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  server->seat->set_idle_inhibitor(false);
  auto inhibitor = server->runtime->fetch<wlr_idle_inhibitor_v1*>(listener);
  server->runtime->remove(inhibitor);
}
} // namespace ura
