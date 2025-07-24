#include "ura/server.hpp"
#include "ura/keyboard.hpp"
#include "ura/runtime.hpp"

namespace ura {
void on_new_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto device = static_cast<wlr_input_device*>(data);
  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
      auto keyboard = new UraKeyboard {};
      keyboard->init(device);
      break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
      server->cursor->attach_device(device);
      break;
    }
    default:
      break;
  }

  // info clients with capabilities
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;

  if (!server->runtime->keyboards.empty()) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat, caps);
}

} // namespace ura
