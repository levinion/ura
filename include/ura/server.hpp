#pragma once

#include <memory>
#include "ura/cursor.hpp"
#include "ura/layer_shell.hpp"
#include "ura/ura.hpp"
#include "ura/lua.hpp"
#include "ura/client.hpp"

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
  wlr_seat* seat;
  wlr_server_decoration_manager* server_decoration_manager;
  wlr_xdg_decoration_manager_v1* decoration_manager;
  wlr_layer_shell_v1* layer_shell;
  wlr_cursor_shape_manager_v1* cursor_shape_manager;
  wlr_output_manager_v1* output_manager;
  wlr_xdg_activation_v1* activation;
  wlr_foreign_toplevel_manager_v1* foreign_manager;
  wlr_text_input_manager_v3* text_input_manager;

  std::unique_ptr<UraRuntime> runtime;
  std::unique_ptr<Lua> lua;
  std::unique_ptr<UraCursor> cursor;

  bool quit = false;

  // Get the global instance of server
  static UraServer* get_instance();
  UraServer* init();

  void run();
  void destroy();
  ~UraServer();

  std::optional<UraClient> foreground_client(double* sx, double* sy);
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
  void setup_activation();
  void setup_foreign();
  void setup_drm();
  void setup_text_input();
};

} // namespace ura
