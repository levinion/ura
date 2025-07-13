#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraKeyboard {
public:
  wl_list link;
  wlr_keyboard* keyboard;

  wl_listener modifiers;
  wl_listener key;
  wl_listener destroy;
};

} // namespace ura
