#include "ura/keyboard.hpp"
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include "ura/config.hpp"
#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/ura.hpp"

namespace ura {

void UraServer::register_keyboard(wlr_input_device* device) {
  auto wlr_keyboard = wlr_keyboard_from_input_device(device);

  auto keyboard = new UraKeyboard {};
  keyboard->keyboard = wlr_keyboard;

  // setup xkb_keymap
  auto xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  auto xkb_keymap =
    xkb_keymap_new_from_names(xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(wlr_keyboard, xkb_keymap);
  xkb_keymap_unref(xkb_keymap);
  xkb_context_unref(xkb_context);

  // set repeat rate
  wlr_keyboard_set_repeat_info(wlr_keyboard, 40, 300);

  this->runtime->register_callback(
    &wlr_keyboard->events.modifiers,
    on_keyboard_modifiers,
    keyboard
  );
  this->runtime
    ->register_callback(&wlr_keyboard->events.key, on_keyboard_key, keyboard);
  this->runtime
    ->register_callback(&device->events.destroy, on_keyboard_destroy, keyboard);

  wlr_seat_set_keyboard(this->seat, keyboard->keyboard);
}

void on_keyboard_modifiers(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  keyboard->notify_modifiers();
}

void on_keyboard_key(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  auto event = static_cast<wlr_keyboard_key_event*>(data);
  if (!keyboard->process_key(event))
    keyboard->notify_key(event);
}

void on_keyboard_destroy(struct wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto keyboard = server->runtime->fetch<UraKeyboard*>(listener);
  server->runtime->remove(keyboard);
  delete keyboard;
}

bool UraKeyboard::process_key(wlr_keyboard_key_event* event) {
  auto server = UraServer::get_instance();
  uint32_t keycode = event->keycode + 8; // xkeycode = libinput keycode + 8
  auto sym = xkb_state_key_get_one_sym(this->keyboard->xkb_state, keycode);
  auto modifiers = wlr_keyboard_get_modifiers(this->keyboard);

  // if keybinding is matched, then not pass it to clients
  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    // switch tty
    if (sym >= XKB_KEY_XF86Switch_VT_1 && sym <= XKB_KEY_XF86Switch_VT_12) {
      auto vt = sym - XKB_KEY_XF86Switch_VT_1 + 1;
      wlr_session_change_vt(server->session, vt);
      return true;
    }

    // exec keybinding
    auto id = (static_cast<uint64_t>(modifiers) << 32) | sym;
    if (server->config->keybinding.contains(id)) {
      server->config->keybinding[id]();
      return true;
    };
  }

  return false;
}
} // namespace ura
