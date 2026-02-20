#include "ura/ura.hpp"
#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/view/client.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/view.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/seat.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/core/lua.hpp"

namespace ura {
void UraSeat::init() {
  auto server = UraServer::get_instance();
  this->seat = wlr_seat_create(server->display, "seat0");
  server->runtime->register_callback(
    &server->backend->events.new_input,
    on_new_input,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_cursor,
    on_seat_request_cursor,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_selection,
    on_seat_request_set_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_primary_selection,
    on_seat_request_set_primary_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_start_drag,
    on_seat_request_start_drag,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.start_drag,
    on_seat_start_drag,
    nullptr
  );
  this->cursor = std::make_unique<UraCursor>();
  this->cursor->init();
  this->text_input = std::make_unique<UraTextInput>();
}

UraToplevel* UraSeat::focused_toplevel() {
  auto client = this->focused_client();
  if (!client || client.value().type != UraSurfaceType::Toplevel)
    return nullptr;
  return client.value().transform<UraToplevel>();
}

// only layer_shell and toplevel can be focused
std::optional<UraClient> UraSeat::focused_client() {
  if (!this->seat->keyboard_state.focused_surface)
    return {};
  auto client = UraClient::from(this->seat->keyboard_state.focused_surface);
  return client;
}

// this function only handles keyboard focus. pointer focus is handled by cursor
void UraSeat::unfocus() {
  auto server = UraServer::get_instance();
  if (!this->seat->keyboard_state.focused_surface)
    return;
  auto client = this->focused_client();
  if (client) {
    if (client->type == UraSurfaceType::Toplevel)
      client->transform<UraToplevel>()->unfocus();
    else if (client->type == UraSurfaceType::LayerShell)
      client->transform<UraLayerShell>()->unfocus();
    server->seat->cursor->set_xcursor("left_ptr");
  }
  wlr_seat_keyboard_notify_clear_focus(seat);
  server->lua->emit_hook("focus-change", {});
}

void UraSeat::focus(UraClient client) {
  if (!client.surface || client.type == UraSurfaceType::Popup
      || client.type == UraSurfaceType::SessionLock
      || client.type == UraSurfaceType::Null
      || client.type == UraSurfaceType::Unknown
      || this->seat->keyboard_state.focused_surface == client.surface)
    return;
  auto focused = this->focused_client();
  if (focused)
    this->unfocus();
  client.focus();
  auto server = UraServer::get_instance();
  server->lua->emit_hook("focus-change", {});
}

void UraSeat::focus(UraToplevel* toplevel) {
  this->focus(UraClient::from(toplevel));
}

void UraSeat::focus(UraLayerShell* layer_shell) {
  this->focus(UraClient::from(layer_shell));
}

void UraSeat::notify_idle_activity() {
  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_notify_activity(server->idle_notifier, this->seat);
  if (!server->idle_notifier->WLR_PRIVATE.inhibited)
    for (auto [_, output] : server->view->outputs) {
      if (!output->dpms_on)
        output->set_dpms_mode(true);
    }
}

void UraSeat::set_idle_inhibitor(bool flag) {
  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, flag);
}

void UraSeat::update_capability() {
  uint32_t caps = 0;
  if (!this->pointers.empty() || !this->tablets.empty()) {
    caps |= WL_SEAT_CAPABILITY_POINTER;
  }
  if (!this->keyboards.empty()) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(this->seat, caps);
}

void UraSeat::attach_new_input(wlr_input_device* device) {
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

  auto server = UraServer::get_instance();

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
      auto keyboard = new UraKeyboard {};
      keyboard->init(device);
      this->keyboards.push_back(keyboard);
      break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
      this->cursor->attach_device(device);
      auto pointer = new UraPointer {};
      pointer->init(device);
      this->pointers.push_back(pointer);
      break;
    }
    case WLR_INPUT_DEVICE_TABLET: {
      this->cursor->attach_device(device);
      auto tablet = new UraTablet {};
      tablet->init(device);
      this->tablets.push_back(tablet);
      break;
    }
    case WLR_INPUT_DEVICE_TOUCH: {
      server->seat->cursor->attach_device(device);
      break;
    }
    default:
      break;
  }

  server->seat->update_capability();

  auto args = flexible::create_table();
  args.set("name", device->name);
  server->lua->emit_hook("new-input", args);
}

uint32_t UraSeat::get_modifiers() {
  uint32_t modifiers = 0u;
  for (auto keyboard : this->keyboards) {
    modifiers |= wlr_keyboard_get_modifiers(keyboard->keyboard);
  }
  return modifiers;
}

} // namespace ura
