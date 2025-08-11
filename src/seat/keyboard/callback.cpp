#include "ura/keyboard.hpp"
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/runtime.hpp"
#include "ura/seat.hpp"
#include "ura/ura.hpp"

namespace ura {

void on_keyboard_modifiers(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  keyboard->process_modifiers();
}

void on_keyboard_key(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  auto event = static_cast<wlr_keyboard_key_event*>(data);
  keyboard->process_key(event);
}

void on_keyboard_destroy(struct wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  server->runtime->remove(keyboard);
  server->seat->keyboards.remove(keyboard);
  delete keyboard;
}

void on_new_virtual_keyboard(wl_listener* listener, void* data) {
  auto virtual_keyboard = static_cast<wlr_virtual_keyboard_v1*>(data);
  auto device = &virtual_keyboard->keyboard.base;
  auto keyboard = new UraKeyboard {};
  keyboard->virt = true;
  keyboard->init(device);
}

void on_new_keyboard_shortcuts_inhibitor(wl_listener* listener, void* data) {
  auto inhibitor = static_cast<wlr_keyboard_shortcuts_inhibitor_v1*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &inhibitor->events.destroy,
    on_keyboard_shortcuts_inhibitor_destroy,
    inhibitor
  );
  wlr_keyboard_shortcuts_inhibitor_v1_activate(inhibitor);
  server->seat->keyboard_shortcuts_inhibited = true;
}

void on_keyboard_shortcuts_inhibitor_destroy(
  wl_listener* listener,
  void* data
) {
  auto server = UraServer::get_instance();
  auto inhibitor =
    server->runtime->fetch<wlr_keyboard_shortcuts_inhibitor_v1*>(listener);
  wlr_keyboard_shortcuts_inhibitor_v1_deactivate(inhibitor);
  server->seat->keyboard_shortcuts_inhibited = false;
  server->runtime->remove(inhibitor);
}

} // namespace ura
