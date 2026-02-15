#pragma once

#include <memory>
#include "ura/ura.hpp"
#include "ura/core/dispatcher.hpp"
#include "ipc.hpp"
#include "ura/util/flexible.hpp"

namespace ura {
// extern
class UraClient;
class UraToplevel;
class UraOutput;
class UraKeyboard;
class UraRuntime;
class UraSeat;
class Lua;
class UraView;

enum class UraGlobalType { Toplevel, Output, LayerShell, Popup };

class UraGlobal {
public:
  UraGlobal() = default;
  UraGlobal(UraGlobalType&& t) : type(t) {};
  UraGlobalType type;
  flexible::object userdata;
};

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
  wlr_relative_pointer_manager_v1* relative_pointer_manager;
  wlr_pointer_constraints_v1* pointer_constraints;
  wlr_tablet_manager_v2* tablet_manager;
  wlr_virtual_pointer_manager_v1* virtual_pointer_manager;
  ura_ipc* ipc;

  std::unique_ptr<UraRuntime> runtime;
  std::unique_ptr<Lua> lua;
  std::unique_ptr<UraSeat> seat;
  std::unique_ptr<UraView> view;
  std::unique_ptr<UraDispatcher<64>> dispatcher;

  std::unordered_map<uint64_t, UraGlobal> globals;

  static UraServer* get_instance();
  UraServer* init();
  void run();
  void destroy();
  void terminate();
  ~UraServer();

private:
  static UraServer* instance;
  bool quit = false;
  void setup_ipc();
  void setup_signal();
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
  void setup_idle();
  void setup_session_lock();
  void setup_others();
};

} // namespace ura
