#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/ura.hpp"

namespace ura {

UraServer* UraServer::init() {
  wlr_log_init(WLR_DEBUG, NULL);

  auto server = new UraServer {};
  server->setup_base();
  server->setup_compositor();
  server->setup_output();
  server->setup_toplevel();
  server->setup_popup();
  server->setup_cursor();
  server->setup_input();

  return server;
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
}

void UraServer::setup_output() {
  // create output layout
  this->output_layout = wlr_output_layout_create(this->display);

  // init outputs
  wl_list_init(&this->outputs);
  // new output handle
  this->new_output.notify = on_new_output;
  wl_signal_add(&this->backend->events.new_output, &this->new_output);

  // create scene
  this->scene = wlr_scene_create();
  this->scene_layout =
    wlr_scene_attach_output_layout(this->scene, this->output_layout);
}

void UraServer::setup_toplevel() {
  wl_list_init(&this->toplevels);
  // create xdg_shell
  this->xdg_shell = wlr_xdg_shell_create(this->display, 3);
  // new toplevel callback
  this->new_xdg_toplevel.notify = on_new_toplevel;
  wl_signal_add(&this->xdg_shell->events.new_toplevel, &this->new_xdg_toplevel);
}

void UraServer::setup_popup() {
  this->new_xdg_popup.notify = on_new_popup;
  wl_signal_add(&this->xdg_shell->events.new_popup, &this->new_xdg_popup);
}

void UraServer::setup_cursor() {
  // create cursor
  this->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(this->cursor, this->output_layout);

  // create cursor manager with cursor size
  this->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  // configure cursor and setup curosr callbacks
  this->cursor_mode = CursorMode::CURSOR_PASSTHROUGH;
  this->cursor_motion.notify = on_cursor_motion;
  wl_signal_add(&this->cursor->events.motion, &this->cursor_motion);
  this->cursor_motion_absolute.notify = on_cursor_motion_absolute;
  wl_signal_add(
    &this->cursor->events.motion_absolute,
    &this->cursor_motion_absolute
  );
  this->cursor_button.notify = on_cursor_button;
  wl_signal_add(&this->cursor->events.button, &this->cursor_button);
  this->cursor_axis.notify = on_cursor_axis;
  wl_signal_add(&this->cursor->events.axis, &this->cursor_axis);
  this->cursor_frame.notify = on_cursor_frame;
  wl_signal_add(&this->cursor->events.frame, &this->cursor_frame);
}

void UraServer::setup_input() {
  wl_list_init(&this->keyboards);

  // new input callback
  this->new_input.notify = on_new_input;
  wl_signal_add(&this->backend->events.new_input, &this->new_input);

  // create seat with name seat0
  this->seat = wlr_seat_create(this->display, "seat0");

  // request cursor callback
  this->request_cursor.notify = on_seat_request_cursor;
  wl_signal_add(&this->seat->events.request_set_cursor, &this->request_cursor);
  // request set selection callback
  this->request_set_selection.notify = on_seat_request_set_selection;
  wl_signal_add(
    &this->seat->events.request_set_selection,
    &this->request_set_selection
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
  this->config_mgr.load_config();

  // run event loop
  wl_display_run(this->display);
}

void UraServer::destroy() {
  wl_display_destroy_clients(this->display);
  wl_list_remove(&this->new_xdg_toplevel.link);
  wl_list_remove(&this->new_xdg_popup.link);

  wl_list_remove(&this->cursor_motion.link);
  wl_list_remove(&this->cursor_motion_absolute.link);
  wl_list_remove(&this->cursor_button.link);
  wl_list_remove(&this->cursor_axis.link);
  wl_list_remove(&this->cursor_frame.link);

  wl_list_remove(&this->new_input.link);
  wl_list_remove(&this->request_cursor.link);
  wl_list_remove(&this->request_set_selection.link);

  wl_list_remove(&this->new_output.link);

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
