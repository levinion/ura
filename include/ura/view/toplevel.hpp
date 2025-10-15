#pragma once

#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include <array>
#include <string>
#include <sol/sol.hpp>

namespace ura {

class UraOutput;
class UraWorkSpace;

class UraToplevel {
public:
  bool mapped = true;
  bool destroying = false;
  bool draggable = false;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  int z_index;
  std::string output;
  UraWorkSpace* workspace;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;
  Vec4<int> geometry;

  // same with css, top > right > bottom > left
  std::array<wlr_scene_rect*, 4> borders;
  std::array<float, 4> active_border_color;
  std::array<float, 4> inactive_border_color;
  uint border_width;

  static UraToplevel* from(wlr_surface* surface);
  static UraToplevel* from(uint32_t id);

  void init(wlr_xdg_toplevel* xdg_toplevel);
  void destroy();
  void commit();
  void focus();
  void unfocus();
  bool move(int x, int y);
  bool resize(int width, int height);
  void center();
  void close();
  void map();
  void unmap();
  std::string title();
  std::string app_id();
  void set_title(std::string title); // inner api
  void set_app_id(std::string app_id); // inner api
  void move_to_workspace(UraWorkSpace* workspace);
  void move_to_workspace(int index);
  void move_to_workspace(std::string name);
  int index();
  void activate();
  void set_z_index(int z);
  bool is_focused();
  sol::table to_lua_table();
  uint64_t id();

private:
  void create_borders();
  void resize_borders(int width, int height);
  void set_border_color(std::array<float, 4>& color);
  void dismiss_popups();
  bool prepared = false;
};

} // namespace ura
