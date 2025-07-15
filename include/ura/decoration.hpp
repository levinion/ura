#pragma once

#include "ura/ura.hpp"

namespace ura {
class UraDecoration {
public:
  wlr_xdg_toplevel_decoration_v1* toplevel_decoration;

  inline void set_decoration(wlr_xdg_toplevel_decoration_v1_mode mode) {
    wlr_xdg_toplevel_decoration_v1_set_mode(this->toplevel_decoration, mode);
  }
};
} // namespace ura
