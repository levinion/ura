#pragma once

#include "ura/ura.hpp"

namespace ura {

class UraToplevel {
public:
  wl_list link;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  wl_listener map;
  wl_listener unmap;
  wl_listener commit;
  wl_listener destroy;
  wl_listener request_move;
  wl_listener request_resize;
  wl_listener request_maximize;
  wl_listener request_fullscreen;

  void focus();
  void move();
  void resize(uint32_t edges);
};

} // namespace ura
