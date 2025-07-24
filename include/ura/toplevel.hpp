#pragma once

#include "ura/ura.hpp"
#include <cassert>
#include <string>

namespace ura {

class UraOutput;
class UraWorkSpace;

class UraToplevel {
public:
  bool mapped = true;
  bool floating = false;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  UraOutput* output;
  UraWorkSpace* workspace;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;

  int floating_width, floating_height;

  static UraToplevel* from(wlr_surface* surface);
  void init(wlr_xdg_toplevel* xdg_toplevel);
  void destroy();
  void commit();
  wlr_box logical_geometry();
  void focus();
  void move(int x, int y);
  void resize(int width, int height);
  void set_fullscreen(bool flag);
  bool fullscreen();
  void toggle_fullscreen();
  void close();
  void map();
  void unmap();
  std::string title();
  void set_title(std::string title);
  bool is_normal();
  int move_to_workspace(int index);
  int index();
  void activate();
  void set_float(bool flag);
};

} // namespace ura
