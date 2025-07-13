#include "ura/keyboard.hpp"
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <print>
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

  keyboard->modifiers.notify = on_keyboard_modifiers;
  wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
  keyboard->key.notify = on_keyboard_key;
  wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);
  keyboard->destroy.notify = on_keyboard_destroy;
  wl_signal_add(&device->events.destroy, &keyboard->destroy);

  wlr_seat_set_keyboard(this->seat, keyboard->keyboard);

  wl_list_insert(&this->keyboards, &keyboard->link);
}

void on_keyboard_modifiers(wl_listener* listener, void* data) {
  UraKeyboard* keyboard = wl_container_of(listener, keyboard, modifiers);

  auto server = UraServer::get_instance();
  // set current keyboard to seat
  wlr_seat_set_keyboard(server->seat, keyboard->keyboard);
  // send modifiers event to clients
  wlr_seat_keyboard_notify_modifiers(
    server->seat,
    &keyboard->keyboard->modifiers
  );
}

void on_keyboard_key(wl_listener* listener, void* data) {
  UraKeyboard* keyboard = wl_container_of(listener, keyboard, key);
  auto event = static_cast<wlr_keyboard_key_event*>(data);
  auto server = UraServer::get_instance();
  auto seat = server->seat;

  // libinput keycode to x key code
  uint32_t keycode = event->keycode + 8;

  // get key syms from keycode
  const xkb_keysym_t* syms;
  auto nsyms =
    xkb_state_key_get_syms(keyboard->keyboard->xkb_state, keycode, &syms);

  auto modifiers = wlr_keyboard_get_modifiers(keyboard->keyboard);

  auto handled = false;

  // try handling with keybindings
  if ((modifiers) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    for (int i = 0; i < nsyms; i++) {
      if (server->process_keybindings(modifiers, syms[i]))
        handled = true;
    }
  }

  // pass keys to clients
  if (!handled) {
    wlr_seat_set_keyboard(seat, keyboard->keyboard);
    wlr_seat_keyboard_notify_key(
      seat,
      event->time_msec,
      event->keycode,
      event->state
    );
  }
}

void on_keyboard_destroy(struct wl_listener* listener, void* data) {
  UraKeyboard* keyboard = wl_container_of(listener, keyboard, destroy);
  wl_list_remove(&keyboard->modifiers.link);
  wl_list_remove(&keyboard->key.link);
  wl_list_remove(&keyboard->destroy.link);
  wl_list_remove(&keyboard->link);
  delete keyboard;
}

bool UraServer::process_keybindings(uint32_t modifier, xkb_keysym_t sym) {
  auto keypair_id = (static_cast<uint64_t>(modifier) << 32) | sym;

  std::println("pressed: {} {} {}", modifier, sym, keypair_id);

  if (this->config_mgr->keybinding.contains(keypair_id)) {
    this->config_mgr->keybinding[keypair_id]();
    return true;
  };

  // fallback, just for test
  if (modifier & WLR_MODIFIER_LOGO && sym == XKB_KEY_Escape) {
    wl_display_terminate(this->display);
  }

  // return false to not pass keys to clients
  return false;
}
} // namespace ura
