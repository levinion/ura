#pragma once

#include "ura/config.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"

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
  wlr_xdg_shell* xdg_shell;

  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;

  wlr_seat* seat;
  CursorMode cursor_mode;
  UraToplevel* grabbed_toplevel;
  double grab_x, grab_y;
  wlr_box grab_geobox;
  uint32_t resize_edges;

  std::unique_ptr<UraConfigManager> config_mgr;
  std::unique_ptr<UraRuntime> runtime;

  // Methods
  static UraServer* get_instance();
  UraServer* init();

  void run();
  void destroy();
  ~UraServer();

  UraToplevel*
  foreground_toplevel(wlr_surface** surface, double* sx, double* sy);
  UraToplevel* focused_toplevel;

  void register_keyboard(wlr_input_device* device);
  void register_pointer(wlr_input_device* device);
  bool process_keybindings(uint32_t modifier, xkb_keysym_t sym);
  void process_cursor_motion(uint32_t time_msec);
  void process_cursor_move();
  void process_cursor_resize();
  void process_cursor_passthrough(uint32_t time_msec);
  void reset_cursor_mode();

  inline void terminate() {
    wl_display_terminate(this->display);
  }

private:
  static UraServer* instance;
  void setup_cursor();
  void setup_input();
  void setup_toplevel();
  void setup_popup();
  void setup_output();
  void setup_compositor();
  void setup_base();
};

} // namespace ura
