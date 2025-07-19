#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/ura.hpp"

namespace ura {
void on_output_manager_apply(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto config = static_cast<wlr_output_configuration_v1*>(data);
  wlr_output_manager_v1_set_configuration(server->output_manager, config);
}
} // namespace ura
