#pragma once

#include "ura/ura.hpp"
#include <cassert>
#include <string>

namespace ura {

class UraOutput;
class UraWorkSpace;

class UraToplevel {
public:
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  UraOutput* output;
  bool mapped = true;
  wlr_xdg_toplevel_decoration_v1* decoration;
  UraWorkSpace* workspace;

  static UraToplevel* from(wlr_xdg_toplevel* toplevel);

  void focus();

  inline bool initialized() {
    return this->xdg_toplevel->base->initialized;
  }

  inline bool initial_commit() {
    return this->xdg_toplevel->base->initial_commit;
  }

  inline void move(int x, int y) {
    wlr_scene_node_set_position(&this->scene_tree->node, x, y);
  }

  inline void resize(int width, int height) {
    wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
  }

  inline void set_fullscreen(bool flag) {
    if (this->xdg_toplevel->base->initialized)
      wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
  }

  inline bool fullscreen() {
    if (!this->xdg_toplevel)
      return false;
    return this->xdg_toplevel->pending.fullscreen;
  }

  inline void toggle_fullscreen() {
    this->set_fullscreen(!this->fullscreen());
  }

  inline void close() {
    wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  }

  inline void map() {
    this->mapped = true;
    wlr_scene_node_set_enabled(&this->scene_tree->node, true);
  }

  inline void unmap() {
    this->mapped = false;
    wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  }

  inline std::string title() {
    return this->xdg_toplevel->title;
  }

  inline void set_title(std::string title) {
    this->xdg_toplevel->title = title.data();
  }

  int move_to_workspace(int index);
};

} // namespace ura
