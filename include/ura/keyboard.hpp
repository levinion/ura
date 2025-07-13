#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraKeyboard {
public:
  wlr_keyboard* keyboard;

  inline void set_repeat(int rate, int delay) {
    wlr_keyboard_set_repeat_info(keyboard, rate, delay);
  }
};

} // namespace ura
