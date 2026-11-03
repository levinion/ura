#pragma once

#include "ura/core/entity.hpp"
#include "ura/core/server.hpp"
#include "ura/scene/components.hpp"
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include "ura/util/rect.hpp"
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

  const WindowLayout& layout();
  const SurfaceContent& content();
  const SceneNode& node();
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
  bool move(int x, int y, const AnimationOptions& options);
  bool resize(int width, int height);
  bool resize(int width, int height, const AnimationOptions& options);
  void animate_opacity(float opacity, const AnimationOptions& options);
  void animate_open(
    const AnimationOptions& options,
    float from_scale,
    float from_opacity,
    float to_scale,
    float to_opacity
  );
  void request_close_animation(
    const AnimationOptions& options,
    float to_scale,
    float to_opacity
  );
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
  const Effect& effect();
  void set_effect(const Effect& effect);

  unsigned int border_width();
  std::string border_color = "#00000000";
  void set_border_color(std::string_view color);

private:
  UraScene& scene_model();
  void dismiss_popups();
  void set_output(UraOutput* output);
  void update_surface_content_from_surface();
  bool check_transition_commit();
  void start_window_animation(bool timed_out);
  void cancel_transition_timer();
  void schedule_transition_timeout(uint64_t generation);
  Rect<double> layout_target_rect();
  void apply_fullscreen_output(UraOutput* output, bool save_restore);
  void apply_pending_fullscreen_output();
  void restore_fullscreen_output();
  UraOutput* foreign_output = nullptr;
  double preferred_scale = -1.0;
  bool fullscreen_output_restore_valid = false;
  Rect<int> fullscreen_output_restore_geometry;
  Vec<std::string> fullscreen_output_restore_tags;
  std::string pending_fullscreen_output;
  std::string cached_title;
  std::string cached_app_id;
  bool prepared = false;
  bool initial_map = true;
  int transition_timer = -1;
  uint64_t pending_close_target = 0;
  AnimationOptions pending_close_options;
  Rect<double> pending_close_bounds;
  float pending_close_scale = 1.f;
  float pending_close_opacity = 0.f;

  template<typename T>
  std::optional<T> get_userdata(std::string_view name) {
    auto server = UraServer::get_instance();
    return server->entities->get_userdata<T>(this->id(), name);
  }
};

} // namespace ura
