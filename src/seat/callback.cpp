#include "flexible/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/seat/keyboard.hpp"
#include "ura/seat/pointer.hpp"
#include "ura/seat/tablet.hpp"
#include "ura/seat/seat.hpp"
#include "ura/ura.hpp"
#include "ura/core/state.hpp"

namespace ura {
void on_new_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto device = static_cast<wlr_input_device*>(data);
  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
      auto keyboard = new UraKeyboard {};
      keyboard->init(device);
      server->seat->keyboards.push_back(keyboard);
      break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
      server->seat->cursor->attach_device(device);
      auto pointer = new UraPointer {};
      pointer->init(device);
      server->seat->pointers.push_back(pointer);
      break;
    }
    case WLR_INPUT_DEVICE_TABLET: {
      server->seat->cursor->attach_device(device);
      auto tablet = new UraTablet {};
      tablet->init(device);
      server->seat->tablets.push_back(tablet);
      break;
    }
    case WLR_INPUT_DEVICE_TOUCH: {
      server->seat->cursor->attach_device(device);
      break;
    }
    default:
      break;
  }

  // info clients with capabilities
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;

  if (!server->seat->keyboards.empty()) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }

  wlr_seat_set_capabilities(server->seat->seat, caps);

  auto args = flexible::create_table();
  args.set("name", device->name);
  server->state->try_execute_hook("new-input", args);
}

} // namespace ura
