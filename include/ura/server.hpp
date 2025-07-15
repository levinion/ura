#pragma once

#include "ura/config.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"
#include "wlr/types/wlr_xdg_decoration_v1.h"
#include "ura/lua.hpp"

namespace ura {
// extern
class UraToplevel;

class UraServer {
public:
  wl_display* display;
  wlr_backend* backend;
  wlr_session* session;
  wlr_renderer* renderer;
  wlr_allocator* allocator;
  wlr_scene* scene;
  wlr_scene_output_layout* scene_layout;
  wlr_output_layout* output_layout;
  wlr_xdg_shell* xdg_shell;
  wlr_cursor* cursor;
  wlr_xcursor_manager* cursor_mgr;
  wlr_seat* seat;
  wlr_xdg_decoration_manager_v1* decoration_manager;

  std::unique_ptr<UraConfig> config;
  std::unique_ptr<UraRuntime> runtime;
  std::unique_ptr<Lua> lua;

  // Methods
  static UraServer* get_instance();
  UraServer* init();

  void run();
  void destroy();
  ~UraServer();

  UraToplevel*
  foreground_toplevel(wlr_surface** surface, double* sx, double* sy);
  UraToplevel* focused_toplevel;

  inline wlr_output_mode* output_mode() {
    auto output = wlr_output_layout_output_at(
      this->output_layout,
      this->cursor->x,
      this->cursor->y
    );
    return output->current_mode;
  }

  void register_keyboard(wlr_input_device* device);
  void register_pointer(wlr_input_device* device);
  void process_cursor_motion(uint32_t time_msec);
  void process_cursor_passthrough(uint32_t time_msec);

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
  void setup_decoration();
};

} // namespace ura
