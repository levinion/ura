#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"

namespace ura {

void on_new_idle_inhibitor(wl_listener* listener, void* data) {
  auto inhibitor = static_cast<wlr_idle_inhibitor_v1*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &inhibitor->events.destroy,
    on_idle_inhibitor_destroy,
    inhibitor
  );
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, 1);
}

void on_idle_inhibitor_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, 0);
  auto inhibitor = server->runtime->fetch<wlr_idle_inhibitor_v1*>(listener);
  server->runtime->remove(inhibitor);
}
} // namespace ura
