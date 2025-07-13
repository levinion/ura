#pragma once

#include "ura/config.hpp"
#include "ura/ura.hpp"

namespace ura {

enum class CursorMode {
  CURSOR_PASSTHROUGH,
  CURSOR_MOVE,
  CURSOR_RESIZE,
};

// extern
class UraToplevel;

class UraServer {
public:
  wl_display* display;
  wlr_backend* backend;
  wlr_renderer* renderer;
  wlr_allocator* allocator;

  wlr_scene* scene;
  wlr_scene_output_layout* scene_layout;

  wlr_output_layout* output_layout;
  wl_list outputs;
  wl_listener new_output;

  wlr_xdg_shell* xdg_shell;
  wl_listener new_xdg_toplevel;
  wl_listener new_xdg_popup;
  wl_list toplevels;

  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  wl_listener cursor_motion;
  wl_listener cursor_motion_absolute;
  wl_listener cursor_button;
  wl_listener cursor_axis;
  wl_listener cursor_frame;

  wlr_seat* seat;
  wl_listener new_input;
  wl_listener request_cursor;
  wl_listener request_set_selection;
  wl_list keyboards;
  CursorMode cursor_mode;
  UraToplevel* grabbed_toplevel;
  double grab_x, grab_y;
  wlr_box grab_geobox;
  uint32_t resize_edges;

  std::unique_ptr<UraConfigManager> config_mgr;

  // Methods
  static UraServer* get_instance();
  UraServer* init();
  void setup_cursor();
  void setup_input();
  void setup_toplevel();
  void setup_popup();
  void setup_output();
  void setup_compositor();
  void setup_base();

  void run();
  void destroy();
  ~UraServer();

  void register_keyboard(wlr_input_device* device);
  void register_pointer(wlr_input_device* device);
  bool process_keybindings(uint32_t modifier, xkb_keysym_t sym);
  void process_cursor_motion(uint32_t time_msec);
  void process_cursor_move();
  void process_cursor_resize();
  void process_cursor_passthrough(uint32_t time_msec);
  void reset_cursor_mode();
  UraToplevel*
  desktop_toplevel_at(wlr_surface** surface, double* sx, double* sy);

private:
  static UraServer* instance;
};

} // namespace ura
