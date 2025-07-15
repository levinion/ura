#pragma once

#include "ura/ura.hpp"
#include <string>

namespace ura {

class UraToplevel {
public:
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;

  bool hidden = false;

  void focus();

  inline void move(int x, int y) {
    wlr_scene_node_set_position(&this->scene_tree->node, x, y);
  }

  inline void resize(int width, int height) {
    wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
  }

  inline void set_fullscreen(bool flag) {
    wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
  }

  inline bool fullscreen() {
    return this->xdg_toplevel->current.fullscreen;
  }

  inline void toggle_fullscreen() {
    this->set_fullscreen(!this->fullscreen());
  }

  inline void close() {
    wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  }

  inline void show() {
    this->hidden = false;
    wlr_scene_node_set_enabled(&this->scene_tree->node, true);
  }

  inline void hide() {
    this->hidden = true;
    wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  }

  inline std::string title() {
    return this->xdg_toplevel->title;
  }

  inline void set_title(std::string title) {
    this->xdg_toplevel->title = title.data();
  }
};

} // namespace ura
