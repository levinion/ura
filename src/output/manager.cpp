#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/ura.hpp"

namespace ura {
void on_output_manager_apply(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto config = static_cast<wlr_output_configuration_v1*>(data);
  size_t states_len;
  wlr_backend_output_state* states =
    wlr_output_configuration_v1_build_state(config, &states_len);
  if (!states) {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
    return;
  }
  if (wlr_backend_commit(server->backend, states, states_len)) {
    wlr_output_configuration_v1_send_succeeded(config);
    wlr_output_manager_v1_set_configuration(server->output_manager, config);
  } else {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
    return;
  }
  delete states;
}
} // namespace ura
