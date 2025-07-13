#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraToplevel {
public:
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;

  void focus();
  void move();
  void resize(uint32_t edges);
  void set_fullscreen(bool flag);

  inline bool fullscreen() {
    return this->xdg_toplevel->current.fullscreen;
  }

  inline void toggle_fullscreen() {
    this->set_fullscreen(!this->fullscreen());
  }

  inline void close() {
    wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  }
};

} // namespace ura
