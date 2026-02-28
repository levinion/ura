#pragma once

#include "ura/core/server.hpp"
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include <string>
#include <sol/sol.hpp>
#include <string_view>

namespace ura {

class UraOutput;

class UraToplevel {
public:
  bool destroying = false;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_scene_tree* scene_tree;
  int z_index;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;
  Vec<std::string> tags;
  uint64_t lru = 0;
  Vec4<int> geometry;

  static UraToplevel* from(wlr_surface* surface);
  static UraToplevel* from(uint64_t id);

  void init(wlr_xdg_toplevel* xdg_toplevel);
  void destroy();
  void commit();
  void focus();
  void unfocus();
  bool move(int x, int y);
  bool resize(int width, int height);
  void center();
  void close();
  bool mapped();
  void map();
  void unmap();
  std::string title();
  std::string app_id();
  void set_title(std::string title); // inner api
  void set_app_id(std::string app_id); // inner api
  void activate();
  void set_z_index(int z);
  bool is_focused();
  uint64_t id();
  void set_fullscreen(bool flag);
  bool is_fullscreen();
  void set_resizing(bool flag);
  bool is_resizing();
  void set_maximized(bool flag);
  bool is_maximized();
  UraOutput* output();
  double scale();
  void set_scale(double scale);
  void update_scale(); // a shortcut of set_scale(scale())
  void set_tags(Vec<std::string>&& tags);
  bool is_tag_matched();

  void create_borders();
  void set_border_color(std::string_view color);
  void resize_borders(int width, int height);
  void move_borders(int x, int y);
  void set_border_invisible(bool flag);

private:
  void dismiss_popups();
  bool prepared = false;
  std::array<wlr_scene_rect*, 4> borders;

  template<typename T>
  std::optional<T> get_userdata(std::string_view name) {
    auto server = UraServer::get_instance();
    if (!server->globals.contains(this->id()))
      return {};
    return server->globals[this->id()].get_userdata<T>(name);
  }
};

} // namespace ura
