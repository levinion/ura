#pragma once

#include "ura/server.hpp"

namespace ura {

class UraKeyboard {
public:
  wlr_keyboard* keyboard;

  static UraKeyboard* from(wlr_keyboard* keyboard);
  void init(wlr_input_device* device);
  void set_repeat(int rate, int delay);
  void process_modifiers();
  void process_key(wlr_keyboard_key_event* event);
  wlr_input_method_keyboard_grab_v2* get_im_grab();
};

} // namespace ura
