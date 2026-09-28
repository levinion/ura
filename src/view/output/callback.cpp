#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/output.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/seat.hpp"
#include "ura/view/view.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data) {
  auto _wlr_output = static_cast<wlr_output*>(data);
  auto output = new UraOutput();
  output->init(_wlr_output);
}

void on_output_frame(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  output->commit();
}

void on_output_request_state(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  auto event = static_cast<wlr_output_event_request_state*>(data);
  wlr_output_commit_state(output->output, event->state);
}

void on_output_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);
  output->destroy();
  delete output;
}

void on_output_manager_apply(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto config = static_cast<wlr_output_configuration_v1*>(data);
  if (!server->view->outputs.empty()) {
    server->view->outputs.begin()->second->apply(config);
    return;
  }
  wlr_output_configuration_v1_send_failed(config);
  wlr_output_configuration_v1_destroy(config);
}

void on_output_layout_change(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  for (auto [_, output] : server->view->outputs)
    output->refresh_geometry();

  bool needs_refocus = false;
  for (auto toplevel : server->view->toplevels) {
    toplevel->update_output();
    if (!toplevel->xdg_toplevel->base->surface->mapped)
      continue;
    if (toplevel->is_tag_matched()) {
      if (!toplevel->mapped())
        toplevel->map();
    } else if (toplevel->mapped()) {
      needs_refocus = needs_refocus || toplevel->is_focused();
      toplevel->unmap();
    }
  }
  if (needs_refocus)
    server->seat->focus_lru();
}

void on_output_power_manager_set_mode(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_output_power_v1_set_mode_event*>(data);
  auto output = UraOutput::from(event->output);
  output->set_dpms_mode(event->mode);
}
} // namespace ura
