#pragma once

#include "ura/server.hpp"

namespace ura {

class UraKeyboard {
public:
  wl_list link;
  UraServer* server;
  wlr_keyboard* keyboard;

  wl_listener modifiers;
  wl_listener key;
  wl_listener destroy;
};

} // namespace ura
