#include "ura/server.hpp"
#include "ura/callback.hpp"

namespace ura {

UraServer* UraServer::init() {
  wlr_log_init(WLR_DEBUG, NULL);

  auto server = new UraServer {};
  // create wayland display
  server->display = wl_display_create();
  // create wayland event_loop
  auto event_loop = wl_display_get_event_loop(server->display);

  // create wlroots backend, renderer and allocator
  server->backend = wlr_backend_autocreate(event_loop, NULL);

  if (server->backend == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_backend");
    exit(1);
  }

  server->renderer = wlr_renderer_autocreate(server->backend);

  if (server->renderer == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_renderer");
    exit(1);
  }

  server->allocator =
    wlr_allocator_autocreate(server->backend, server->renderer);

  if (server->allocator == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr_allocator");
    exit(1);
  }

  // create compositor
  wlr_compositor_create(server->display, 5, server->renderer);
  wlr_subcompositor_create(server->display);
  wlr_data_device_manager_create(server->display);

  // create output layout
  server->output_layout = wlr_output_layout_create(server->display);
  wl_list_init(&server->outputs);
  // new output handle
  server->new_output.notify = on_new_output;
  wl_signal_add(&server->backend->events.new_output, &server->new_output);

  // create scene
  server->scene = wlr_scene_create();
  server->scene_layout =
    wlr_scene_attach_output_layout(server->scene, server->output_layout);

  // create toplevel
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  // new toplevel handle
  server->new_xdg_toplevel.notify = on_new_toplevel;
  wl_signal_add(
    &server->xdg_shell->events.new_toplevel,
    &server->new_xdg_toplevel
  );
  // new popup handle
  server->new_xdg_popup.notify = on_new_popup;
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_xdg_popup);

  // create cursor
  server->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
  server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

  // configure cursor and setup curosr hooks
  server->cursor_mode = CursorMode::CURSOR_PASSTHROUGH;
  server->cursor_motion.notify = on_cursor_motion;
  wl_signal_add(&server->cursor->events.motion, &server->cursor_motion);
  server->cursor_motion_absolute.notify = on_cursor_motion_absolute;
  wl_signal_add(
    &server->cursor->events.motion_absolute,
    &server->cursor_motion_absolute
  );
  server->cursor_button.notify = on_cursor_button;
  wl_signal_add(&server->cursor->events.button, &server->cursor_button);
  server->cursor_axis.notify = on_cursor_axis;
  wl_signal_add(&server->cursor->events.axis, &server->cursor_axis);
  server->cursor_frame.notify = on_cursor_frame;
  wl_signal_add(&server->cursor->events.frame, &server->cursor_frame);

  // create and setup seat
  wl_list_init(&server->keyboards);
  server->new_input.notify = on_new_input;
  wl_signal_add(&server->backend->events.new_input, &server->new_input);
  server->seat = wlr_seat_create(server->display, "seat0");
  server->request_cursor.notify = on_seat_request_cursor;
  wl_signal_add(
    &server->seat->events.request_set_cursor,
    &server->request_cursor
  );
  server->request_set_selection.notify = on_seat_request_set_selection;
  wl_signal_add(
    &server->seat->events.request_set_selection,
    &server->request_set_selection
  );

  return server;
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
