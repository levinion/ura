#include "ura/keyboard.hpp"
#include "ura/seat.hpp"
#include "ura/callback.hpp"
#include "ura/runtime.hpp"

namespace ura {

UraKeyboard* UraKeyboard::from(wlr_keyboard* keyboard) {
  auto& keyboards = UraServer::get_instance()->seat->keyboards;
  return *std::find_if(keyboards.begin(), keyboards.end(), [&](auto i) {
    return i->keyboard == keyboard;
  });
}

void UraKeyboard::set_repeat(int rate, int delay) {
  wlr_keyboard_set_repeat_info(this->keyboard, rate, delay);
}

void UraKeyboard::init(wlr_input_device* device) {
  auto keyboard = wlr_keyboard_from_input_device(device);
  this->keyboard = keyboard;
  // setup xkb_keymap
  auto xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  auto xkb_keymap =
    xkb_keymap_new_from_names(xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
  wlr_keyboard_set_keymap(keyboard, xkb_keymap);
  xkb_keymap_unref(xkb_keymap);
  xkb_context_unref(xkb_context);
  // set repeat rate
  wlr_keyboard_set_repeat_info(keyboard, 40, 300);
  // register callbacks
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &keyboard->events.modifiers,
    on_keyboard_modifiers,
    this
  );
  server->runtime
    ->register_callback(&keyboard->events.key, on_keyboard_key, this);
  server->runtime
    ->register_callback(&device->events.destroy, on_keyboard_destroy, this);
  // attach this keyboard to seat
  wlr_seat_set_keyboard(server->seat->seat, keyboard);

  server->seat->keyboards.push_back(this);
}

void UraKeyboard::process_modifiers() {
  auto server = UraServer::get_instance();
  // send modifier to im grab
  auto kb_grab = this->get_im_grab();
  if (kb_grab) {
    wlr_input_method_keyboard_grab_v2_set_keyboard(kb_grab, this->keyboard);
    wlr_input_method_keyboard_grab_v2_send_modifiers(
      kb_grab,
      &this->keyboard->modifiers
    );
    return;
  }
  // send modifier to client
  wlr_seat_set_keyboard(server->seat->seat, this->keyboard);
  wlr_seat_keyboard_notify_modifiers(
    server->seat->seat,
    &this->keyboard->modifiers
  );
}

void UraKeyboard::process_key(wlr_keyboard_key_event* event) {
  auto server = UraServer::get_instance();
  uint32_t keycode = event->keycode + 8; // xkeycode = libinput keycode + 8
  auto sym = xkb_state_key_get_one_sym(this->keyboard->xkb_state, keycode);
  auto modifiers = wlr_keyboard_get_modifiers(this->keyboard);

  // order: tty > keybinding > input_method > client

  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    // switch tty
    if (sym >= XKB_KEY_XF86Switch_VT_1 && sym <= XKB_KEY_XF86Switch_VT_12) {
      auto vt = sym - XKB_KEY_XF86Switch_VT_1 + 1;
      wlr_session_change_vt(server->session, vt);
      return;
    }
    // exec keybinding
    auto id = (static_cast<uint64_t>(modifiers) << 32) | sym;
    if (server->lua->try_execute_keybinding(id))
      return;
  }

  // handle input method grab
  auto kb_grab = this->get_im_grab();
  if (kb_grab) {
    wlr_input_method_keyboard_grab_v2_set_keyboard(kb_grab, this->keyboard);
    wlr_input_method_keyboard_grab_v2_send_key(
      kb_grab,
      event->time_msec,
      event->keycode,
      event->state
    );
    return;
  }

  // notify client
  wlr_seat_set_keyboard(server->seat->seat, this->keyboard);
  wlr_seat_keyboard_notify_key(
    server->seat->seat,
    event->time_msec,
    event->keycode,
    event->state
  );
}

wlr_input_method_keyboard_grab_v2* UraKeyboard::get_im_grab() {
  auto server = UraServer::get_instance();
  auto input_method = server->seat->text_input->input_method;
  auto virtual_keyboard =
    wlr_input_device_get_virtual_keyboard(&this->keyboard->base);
  if (!input_method || !input_method->keyboard_grab || (virtual_keyboard &&
                wl_resource_get_client(virtual_keyboard->resource) ==
                wl_resource_get_client(input_method->keyboard_grab->resource))) {
    return nullptr;
  }
  return input_method->keyboard_grab;
}
} // namespace ura
