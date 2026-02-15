#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"
#include "ura/seat/keyboard.hpp"
#include "ura/seat/pointer.hpp"
#include "ura/seat/tablet.hpp"
#include "ura/seat/seat.hpp"
#include "ura/ura.hpp"

namespace ura {
void on_new_input(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto device = static_cast<wlr_input_device*>(data);

  // TODO: enable libinput lua plugins
  // This should be called before iterating devices,
  // which means it should be implemented by wlroots' libinput backend.
  // We could do nothing without wlroots impl that.

  // static auto libinput_plugin_loaded = false;
  // if (!libinput_plugin_loaded && wlr_input_device_is_libinput(device)) {
  //   auto handle = wlr_libinput_get_device_handle(device);
  //   auto context = libinput_device_get_context(handle);
  //   libinput_plugin_system_append_default_paths(context);
  //   if (libinput_plugin_system_load_plugins(
  //         context,
  //         LIBINPUT_PLUGIN_SYSTEM_FLAG_NONE
  //       )
  //       == 0) {
  //     libinput_plugin_loaded = true;
  //     log::info("LIBINPUT LUA PLUGINS LOADED");
  //   } else {
  //     log::error("FAILED TO LOAD LIBINPUT LUA PLUGINS");
  //   }
  // }

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
  server->lua->emit_hook("new-input", args);
}

} // namespace ura
