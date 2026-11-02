#pragma once

#include "ura/core/entity.hpp"
#include "ura/core/server.hpp"
#include "ura/scene/components.hpp"
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include <string>
#include <sol/sol.hpp>
#include <string_view>

namespace ura {

class UraOutput;
class UraScene;

class UraToplevel {
public:
  entt::entity entity { entt::null };
  bool destroying = false;
  wlr_xdg_toplevel* xdg_toplevel;
  wlr_xdg_toplevel_decoration_v1* decoration;
  wlr_foreign_toplevel_handle_v1* foreign_handle;
  uint64_t lru = 0;

  const Transform& transform();
  const Geometry& geometry();
  const Visibility& visibility();
  const ZOrder& z_order();
  const WindowTags& tags();
  const Opacity& opacity();

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
  void set_fullscreen_output(UraOutput* output);
  bool is_fullscreen();
  void set_resizing(bool flag);
  bool is_resizing();
  void set_maximized(bool flag);
  bool is_maximized();
  UraOutput* output();
  void update_output();
  void output_destroyed(UraOutput* output);
  double scale();
  void set_scale(double scale);
  void set_tags(Vec<std::string>&& tags);
  bool is_tag_matched();
  void set_opacity(float opacity);

  unsigned int border_width();
  std::string border_color = "#00000000";
  void set_border_color(std::string_view color);

private:
  UraScene& scene_model();
  void dismiss_popups();
  void set_output(UraOutput* output);
  void apply_fullscreen_output(UraOutput* output, bool save_restore);
  void apply_pending_fullscreen_output();
  void restore_fullscreen_output();
  UraOutput* foreign_output = nullptr;
  double preferred_scale = -1.0;
  bool fullscreen_output_restore_valid = false;
  Vec4<int> fullscreen_output_restore_geometry;
  Vec<std::string> fullscreen_output_restore_tags;
  std::string pending_fullscreen_output;
  bool prepared = false;
  bool initial_map = true;

  template<typename T>
  std::optional<T> get_userdata(std::string_view name) {
    auto server = UraServer::get_instance();
    return server->entities->get_userdata<T>(this->id(), name);
  }
};

} // namespace ura
