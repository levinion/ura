#include "ura/core/server.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/seat/seat.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
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
  server->runtime->remove(output);
  server->view->outputs.remove(output);
  delete output;
}

void on_output_manager_apply(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto config = static_cast<wlr_output_configuration_v1*>(data);
  size_t states_len;
  wlr_backend_output_state* states =
    wlr_output_configuration_v1_build_state(config, &states_len);
  if (!states) {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
    return;
  }
  if (wlr_backend_commit(server->backend, states, states_len)) {
    wlr_output_configuration_v1_send_succeeded(config);
    wlr_output_manager_v1_set_configuration(server->output_manager, config);
    auto output = server->view->current_output();
    output->configure_layers();
  } else {
    wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
  }
  delete states;
}

void on_output_power_manager_set_mode(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_output_power_v1_set_mode_event*>(data);
  auto output = UraOutput::from(event->output);
  output->set_dpms_mode(event->mode);
}
} // namespace ura
