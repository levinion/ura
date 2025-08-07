#include <cassert>
#include <memory>
#include "ura/cursor.hpp"
#include "ura/ipc.hpp"
#include "ura/seat.hpp"
#include "ura/server.hpp"
#include "ura/callback.hpp"
#include "ura/runtime.hpp"
#include "ura/ura.hpp"
#include "ura/workspace.hpp"
#include "ura/lua.hpp"
#include <sys/epoll.h>

namespace ura {

UraServer* UraServer::init() {
  wlr_log_init(WLR_DEBUG, NULL);
  this->runtime = UraRuntime::init();
  this->lua = Lua::init();
  assert(this->lua->try_execute_init());
  this->lua->try_execute_hook("prepare");
  this->setup_base();
  this->setup_drm();
  this->setup_compositor();
  this->setup_output();
  this->setup_toplevel();
  this->setup_popup();
  this->setup_decoration();
  this->setup_layer_shell();
  this->setup_seat();
  this->setup_idle();
  this->setup_session_lock();
  this->setup_activation();
  this->setup_foreign();
  this->setup_text_input();
  this->setup_scratchpad();
  this->setup_others();
  return this;
}

void UraServer::setup_base() {
  this->display = wl_display_create();
  auto event_loop = wl_display_get_event_loop(this->display);
  this->backend = wlr_backend_autocreate(event_loop, &this->session);
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
}

void UraServer::setup_drm() {
  wlr_drm_lease_v1_manager_create(this->display, this->backend);
  auto drm_fd = wlr_renderer_get_drm_fd(this->renderer);
  if (drm_fd > 0)
    wlr_linux_drm_syncobj_manager_v1_create(this->display, 1, drm_fd);
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

  // output_manager_v1
  wlr_xdg_output_manager_v1_create(this->display, this->output_layout);
  this->output_manager = wlr_output_manager_v1_create(this->display);
  this->runtime->register_callback(
    &this->output_manager->events.apply,
    on_output_manager_apply,
    nullptr
  );
  // output_power_manager_v1
  this->output_power_manager =
    wlr_output_power_manager_v1_create(this->display);
  this->runtime->register_callback(
    &this->output_power_manager->events.set_mode,
    on_output_power_manager_set_mode,
    nullptr
  );
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

void UraServer::setup_seat() {
  this->seat = std::make_unique<UraSeat>();
  this->seat->init();
}

void UraServer::setup_decoration() {
  this->server_decoration_manager =
    wlr_server_decoration_manager_create(this->display);

  wlr_server_decoration_manager_set_default_mode(
    server_decoration_manager,
    WLR_SERVER_DECORATION_MANAGER_MODE_SERVER
  );
  this->runtime->register_callback(
    &this->server_decoration_manager->events.new_decoration,
    on_new_server_decoration,
    nullptr
  );
  this->decoration_manager =
    wlr_xdg_decoration_manager_v1_create(this->display);
  this->runtime->register_callback(
    &this->decoration_manager->events.new_toplevel_decoration,
    on_new_toplevel_decoration,
    nullptr
  );
}

void UraServer::setup_layer_shell() {
  this->layer_shell = wlr_layer_shell_v1_create(this->display, 1);
  this->runtime->register_callback(
    &this->layer_shell->events.new_surface,
    on_layer_shell_new_surface,
    nullptr
  );
}

void UraServer::setup_activation() {
  this->activation = wlr_xdg_activation_v1_create(this->display);
  this->runtime->register_callback(
    &this->activation->events.request_activate,
    on_activation_request_activate,
    nullptr
  );
}

void UraServer::setup_foreign() {
  auto foreign_registry = wlr_xdg_foreign_registry_create(this->display);
  wlr_xdg_foreign_v1_create(this->display, foreign_registry);
  wlr_xdg_foreign_v2_create(this->display, foreign_registry);
  this->foreign_manager = wlr_foreign_toplevel_manager_v1_create(this->display);
}

void UraServer::setup_text_input() {
  this->text_input_manager = wlr_text_input_manager_v3_create(this->display);
  this->runtime->register_callback(
    &this->text_input_manager->events.text_input,
    on_new_text_input,
    nullptr
  );

  this->input_method_manager =
    wlr_input_method_manager_v2_create(this->display);
  this->runtime->register_callback(
    &this->input_method_manager->events.input_method,
    on_new_input_method,
    nullptr
  );

  this->virtual_keyboard_manager =
    wlr_virtual_keyboard_manager_v1_create(this->display);
  this->runtime->register_callback(
    &this->virtual_keyboard_manager->events.new_virtual_keyboard,
    on_new_virtual_keyboard,
    nullptr
  );
}

void UraServer::setup_idle() {
  this->idle_notifier = wlr_idle_notifier_v1_create(this->display);
  wlr_idle_inhibit_v1_create(this->display);
}

void UraServer::setup_session_lock() {
  this->session_lock_manager =
    wlr_session_lock_manager_v1_create(this->display);
  this->runtime->register_callback(
    &this->session_lock_manager->events.new_lock,
    on_new_session_lock,
    nullptr
  );
}

// scratchpad is a special workspace that cannot be switch to
void UraServer::setup_scratchpad() {
  this->scratchpad = UraWorkSpace::init();
  this->scratchpad->output = nullptr;
}

void UraServer::setup_others() {
  wlr_data_device_manager_create(this->display);
  wlr_linux_dmabuf_v1_create_with_renderer(this->display, 4, this->renderer);
  wlr_export_dmabuf_manager_v1_create(this->display);
  wlr_primary_selection_v1_device_manager_create(this->display);
  wlr_shm_create_with_renderer(this->display, 2, this->renderer);
  wlr_fractional_scale_manager_v1_create(this->display, 1);
  wlr_viewporter_create(this->display);
  wlr_single_pixel_buffer_manager_v1_create(this->display);
  wlr_screencopy_manager_v1_create(this->display);
  wlr_data_control_manager_v1_create(this->display);
  wlr_presentation_create(this->display, this->backend, 2);
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

  // run ipc
  auto ipc = UraIPC::init();

  // set env
  setenv("WAYLAND_DISPLAY", socket, true);
  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

  // run event loop
  auto event_loop = wl_display_get_event_loop(this->display);
  auto wl_fd = wl_event_loop_get_fd(event_loop);
  auto ipc_fd = ipc->fd;

  int epoll_fd = epoll_create1(0);
  assert(epoll_fd != -1);

  int ret;
  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = wl_fd;
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wl_fd, &event);
  assert(ret != -1);
  event.events = EPOLLIN;
  event.data.fd = ipc_fd;
  ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ipc_fd, &event);
  assert(ret != -1);
  epoll_event events[10];

  assert(this->lua->try_execute_hook("ready"));

  while (!this->quit) {
    int nfds = epoll_wait(epoll_fd, events, 10, -1);
    if (nfds == -1) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    for (int i = 0; i < nfds; i++) {
      auto current_fd = events[i].data.fd;
      if (current_fd == wl_fd) {
        if (wl_event_loop_dispatch(event_loop, 0) == -1)
          return;
        wl_display_flush_clients(this->display);
      } else if (current_fd == ipc_fd) {
        ipc->try_handle();
      }
    }
  }
}

void UraServer::destroy() {
  wl_display_destroy_clients(this->display);
  this->seat->cursor->destroy();
  wlr_allocator_destroy(this->allocator);
  wlr_renderer_destroy(this->renderer);
  wlr_backend_destroy(this->backend);
  this->runtime->remove(nullptr);
  wl_display_destroy(this->display);
  wlr_scene_node_destroy(&this->scene->tree.node);
}

UraServer::~UraServer() {
  this->destroy();
}

} // namespace ura
