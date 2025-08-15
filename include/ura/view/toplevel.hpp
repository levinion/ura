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
  bool initial_commit = true;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  int z_index;
  UraOutput* output;
  UraWorkSpace* workspace;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;
  Vec4<int> geometry = { 0, 0, 0, 0 };

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
  bool move(int x, int y, bool force_update_border = false);
  bool resize(int width, int height);
  void center();
  void close();
  void map();
  void unmap();
  std::string title();
  std::string app_id();
  void set_title(std::string title);
  std::optional<int> move_to_workspace(int index);
  void move_to_scratchpad();
  int index();
  void activate();
  void set_layer(int z);
  void request_commit();
  bool is_active();
  sol::table to_lua_table();
  void set_layout(std::string layout);
  void recover_layout();

private:
  void create_borders();
  void set_border_color(std::array<float, 4>& color);
  std::optional<Vec4<int>> apply_layout();
};

} // namespace ura
