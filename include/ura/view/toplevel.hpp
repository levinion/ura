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
  std::string layout = "tiling";
  std::optional<std::string> last_layout;
  bool prepared = false;
  bool first_commit_after_layout_change = true;
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
  void set_title(std::string title);
  void move_to_workspace(int index);
  void move_to_workspace(std::string name);
  int index();
  void activate();
  void set_z_index(int z);
  void redraw(bool recursive = false);
  void redraw_all_others();
  bool is_active();
  sol::table to_lua_table();
  void set_layout(std::string layout);

private:
  void create_borders();
  void set_border_color(std::array<float, 4>& color);
  void apply_layout(bool recursive);
  std::unordered_map<std::string, Vec4<int>> layout_geometry;
};

} // namespace ura
