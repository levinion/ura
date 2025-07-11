#pragma once

#include "ura/server.hpp"

namespace ura {

class UraOutput {
public:
  wl_list link;
  UraServer* server;
  wlr_output* output;
  wl_listener frame;
  wl_listener request_state;
  wl_listener destroy;
};

} // namespace ura
