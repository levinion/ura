#include "ura/seat/seat.hpp"
#include "ura/core/server.hpp"
#include "ura/ura.hpp"

namespace ura {
void on_new_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto device = static_cast<wlr_input_device*>(data);
  server->seat->attach_new_input(device);
}

} // namespace ura
