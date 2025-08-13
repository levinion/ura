#pragma once

#include <memory>
#include "ura/ura.hpp"
#include "ura/view/view.hpp"
#include "ura/view/workspace.hpp"

namespace ura {
// extern
class UraClient;
class UraToplevel;
class UraOutput;
class UraKeyboard;
class UraRuntime;
class UraSeat;
class Lua;

class UraServer {
public:
  wl_display* display;
  wlr_backend* backend;
  wlr_session* session;
  wlr_renderer* renderer;
  wlr_allocator* allocator;
  wlr_scene_output_layout* scene_layout;
  wlr_output_layout* output_layout;
  wlr_xdg_shell* xdg_shell;
  wlr_server_decoration_manager* server_decoration_manager;
  wlr_xdg_decoration_manager_v1* decoration_manager;
  wlr_layer_shell_v1* layer_shell;
  wlr_cursor_shape_manager_v1* cursor_shape_manager;
  wlr_output_manager_v1* output_manager;
  wlr_output_power_manager_v1* output_power_manager;
  wlr_idle_notifier_v1* idle_notifier;
  wlr_idle_inhibit_manager_v1* idle_inhibit_manager;
  wlr_xdg_activation_v1* activation;
  wlr_foreign_toplevel_manager_v1* foreign_manager;
  wlr_text_input_manager_v3* text_input_manager;
  wlr_input_method_manager_v2* input_method_manager;
  wlr_virtual_keyboard_manager_v1* virtual_keyboard_manager;
  wlr_session_lock_manager_v1* session_lock_manager;
  wlr_keyboard_shortcuts_inhibit_manager_v1* keyboard_shortcuts_inhibit_manager;

  std::unique_ptr<UraRuntime> runtime;
  std::unique_ptr<Lua> lua;
  std::unique_ptr<UraSeat> seat;
  std::unique_ptr<UraWorkSpace> scratchpad;
  std::unique_ptr<UraView> view;

  bool quit = false;

  // Get the global instance of server
  static UraServer* get_instance();
  UraServer* init();

  void run();
  void destroy();
  ~UraServer();

  std::optional<UraClient> foreground_client(double* sx, double* sy);
  UraOutput* current_output();
  void terminate();
  void update_output_configuration();

private:
  static UraServer* instance;

  void setup_log();
  void setup_seat();
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
  void setup_scratchpad();
  void setup_idle();
  void setup_session_lock();
  void setup_others();
};

} // namespace ura
