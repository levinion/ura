#pragma once

#include "ura/config.hpp"
#include "ura/layer_shell.hpp"
#include "ura/ura.hpp"
#include "ura/lua.hpp"

namespace ura {
// extern
class UraToplevel;
class UraOutput;
class UraKeyboard;
class UraRuntime;

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
  wlr_layer_shell_v1* layer_shell;
  wlr_cursor_shape_manager_v1* cursor_shape_manager;
  wlr_output_manager_v1* output_manager;

  std::unique_ptr<UraConfig> config;
  std::unique_ptr<UraRuntime> runtime;
  std::unique_ptr<Lua> lua;

  UraToplevel* focused_toplevel;
  UraToplevel* prev_focused_toplevel;

  // Get the global instance of server
  static UraServer* get_instance();
  UraServer* init();

  void run();
  void destroy();
  ~UraServer();

  UraToplevel* foreground_toplevel(double* sx, double* sy);
  UraLayerShell* foreground_layer_shell(double* sx, double* sy);
  wlr_xdg_popup* foreground_popup(double* sx, double* sy);
  UraOutput* current_output();
  UraKeyboard* current_keyboard();

  void process_cursor_motion(uint32_t time_msec);
  void terminate();

  void update_output_configuration();

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
  void setup_layer_shell();
};

} // namespace ura
