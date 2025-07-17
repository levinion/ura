#pragma once

#include "ura/server.hpp"
#include "ura/runtime.hpp"

namespace ura {

class UraKeyboard {
public:
  wlr_keyboard* keyboard;

  static UraKeyboard* from(wlr_keyboard* keyboard) {
    auto keyboards = UraServer::get_instance()->runtime->keyboards;
    return *std::find_if(keyboards.begin(), keyboards.end(), [&](auto i) {
      return i->keyboard == keyboard;
    });
  }

  void init(wlr_input_device* device);

  inline void set_repeat(int rate, int delay) {
    wlr_keyboard_set_repeat_info(keyboard, rate, delay);
  }

  inline void notify_modifiers() {
    auto server = UraServer::get_instance();
    wlr_seat_set_keyboard(server->seat, this->keyboard);
    wlr_seat_keyboard_notify_modifiers(
      server->seat,
      &this->keyboard->modifiers
    );
  }

  inline void notify_key(wlr_keyboard_key_event* event) {
    auto server = UraServer::get_instance();
    wlr_seat_set_keyboard(server->seat, this->keyboard);
    wlr_seat_keyboard_notify_key(
      server->seat,
      event->time_msec,
      event->keycode,
      event->state
    );
  }

  bool process_key(wlr_keyboard_key_event* event);
};

} // namespace ura
