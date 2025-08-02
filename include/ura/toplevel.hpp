#pragma once

#include "ura/ura.hpp"
#include <array>
#include <string>
#include <sol/sol.hpp>

namespace ura {

class UraOutput;
class UraWorkSpace;

class UraToplevel {
public:
  bool mapped = true;
  bool floating = false;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  wlr_scene_tree* layer;
  UraOutput* output;
  UraWorkSpace* workspace;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;
  int floating_width, floating_height;
  wlr_box geometry = { 0, 0, 800, 600 };

  // same with css, top > right > bottom > left
  std::array<wlr_scene_rect*, 4> borders;
  std::array<float, 4> active_border_color;
  std::array<float, 4> inactive_border_color;
  uint border_width;

  static UraToplevel* from(wlr_surface* surface);
  void init(wlr_xdg_toplevel* xdg_toplevel);
  void destroy();
  void commit();
  void focus();
  void unfocus();
  void move(int x, int y);
  void resize(int width, int height);
  void set_fullscreen(bool flag);
  bool fullscreen();
  void toggle_fullscreen();
  void close();
  void map();
  void unmap();
  std::string title();
  std::string app_id();
  void set_title(std::string title);
  bool is_normal();
  bool move_to_workspace(int index);
  void move_to_scratchpad();
  int index();
  void activate();
  void set_float(bool flag);
  void set_layer(wlr_scene_tree* layer);
  void request_commit();
  bool is_active();
  sol::table to_lua_table();

private:
  bool commit_fullscreen();
  bool commit_floating();
  bool commit_normal();
  void create_borders();
  void set_border_color(std::array<float, 4>& color);
};

} // namespace ura
