#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/config.hpp"
#include "ura/ura.hpp"
#include "wlr/types/wlr_output_power_management_v1.h"
#include "wlr/types/wlr_xdg_decoration_v1.h"

namespace ura {

UraServer* UraServer::instance = nullptr;

UraServer* UraServer::get_instance() {
  if (UraServer::instance == nullptr) {
    UraServer::instance = new UraServer {};
  }
  return UraServer::instance;
}

UraServer* UraServer::init() {
  wlr_log_init(WLR_DEBUG, NULL);
  this->runtime = UraRuntime::init();
  this->setup_base();
  this->setup_compositor();
  this->setup_output();
  this->setup_toplevel();
  this->setup_popup();
  this->setup_cursor();
  this->setup_input();
  this->config_mgr = UraConfigManager::init();
  return this;
}

void UraServer::setup_base() {
  this->display = wl_display_create();
  auto event_loop = wl_display_get_event_loop(this->display);
  this->backend = wlr_backend_autocreate(event_loop, nullptr);
  if (!this->backend) {
    wlr_log(WLR_ERROR, "failed to create wlr_backend");
    exit(1);
  }
  this->renderer = wlr_renderer_autocreate(this->backend);
  if (!this->renderer) {
    wlr_log(WLR_ERROR, "failed to create wlr_renderer");
    exit(1);
  }
  this->allocator = wlr_allocator_autocreate(this->backend, this->renderer);
  if (!this->allocator) {
    wlr_log(WLR_ERROR, "failed to create wlr_allocator");
    exit(1);
  }
}

void UraServer::setup_compositor() {
  wlr_compositor_create(this->display, 5, this->renderer);
  wlr_subcompositor_create(this->display);
  wlr_data_device_manager_create(this->display);
  wlr_linux_dmabuf_v1_create_with_renderer(this->display, 4, this->renderer);
  wlr_shm_create_with_renderer(this->display, 2, this->renderer);
  wlr_layer_shell_v1_create(this->display, 1);
  // TODO: register event
  wlr_output_manager_v1_create(this->display);
  wlr_output_power_manager_v1_create(this->display);
  wlr_xdg_decoration_manager_v1_create(this->display);
}

void UraServer::setup_output() {
  // create output layout
  this->output_layout = wlr_output_layout_create(this->display);

  this->runtime->register_callback(
    &this->backend->events.new_output,
    on_new_output,
    nullptr
  );

  // create scene
  this->scene = wlr_scene_create();
  this->scene_layout =
    wlr_scene_attach_output_layout(this->scene, this->output_layout);
}

void UraServer::setup_toplevel() {
  // create xdg_shell
  this->xdg_shell = wlr_xdg_shell_create(this->display, 3);
  // new toplevel callback
  this->runtime->register_callback(
    &this->xdg_shell->events.new_toplevel,
    on_new_toplevel,
    nullptr
  );
}

void UraServer::setup_popup() {
  this->runtime->register_callback(
    &this->xdg_shell->events.new_popup,
    on_new_popup,
    nullptr
  );
}

void UraServer::setup_cursor() {
  // create cursor
  this->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(this->cursor, this->output_layout);

  // create cursor manager with cursor size
  this->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  // configure cursor and setup curosr callbacks
  this->cursor_mode = CursorMode::CURSOR_PASSTHROUGH;

  // register callbacks
  this->runtime->register_callback(
    &this->cursor->events.motion,
    on_cursor_motion,
    nullptr
  );

  this->runtime->register_callback(
    &this->cursor->events.motion_absolute,
    on_cursor_motion_absolute,
    nullptr
  );

  this->runtime->register_callback(
    &this->cursor->events.button,
    on_cursor_button,
    nullptr
  );

  this->runtime
    ->register_callback(&this->cursor->events.axis, on_cursor_axis, nullptr);

  this->runtime
    ->register_callback(&this->cursor->events.frame, on_cursor_frame, nullptr);
}

void UraServer::setup_input() {
  // new input callback

  this->runtime->register_callback(
    &this->backend->events.new_input,
    on_new_input,
    nullptr
  );

  // create seat with name seat0
  this->seat = wlr_seat_create(this->display, "seat0");

  this->runtime->register_callback(
    &this->seat->events.request_set_cursor,
    on_seat_request_cursor,
    nullptr
  );

  this->runtime->register_callback(
    &this->seat->events.request_set_selection,
    on_seat_request_set_selection,
    nullptr
  );
}

void UraServer::run() {
  // create wayland socket
  auto socket = wl_display_add_socket_auto(this->display);
  if (!socket) {
    wlr_backend_destroy(this->backend);
    exit(1);
  }

  // start backend
  if (!wlr_backend_start(this->backend)) {
    wlr_backend_destroy(this->backend);
    wl_display_destroy(this->display);
    exit(1);
  }

  // set env
  setenv("WAYLAND_DISPLAY", socket, true);
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // load config
  this->config_mgr->load_config();

  // run event loop
  wl_display_run(this->display);
}

void UraServer::destroy() {
  wl_display_destroy_clients(this->display);
  wlr_scene_node_destroy(&this->scene->tree.node);
  wlr_xcursor_manager_destroy(this->cursor_mgr);
  wlr_cursor_destroy(this->cursor);
  wlr_allocator_destroy(this->allocator);
  wlr_renderer_destroy(this->renderer);
  wlr_backend_destroy(this->backend);
  wl_display_destroy(this->display);
}

UraServer::~UraServer() {
  this->destroy();
}

} // namespace ura
