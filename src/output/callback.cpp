#include "ura/server.hpp"
#include "ura/output.hpp"
#include <list>
#include "ura/callback.hpp"
#include "ura/ura.hpp"
#include "ura/runtime.hpp"

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
  output->fresh_screen();
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
  server->runtime->outputs.remove(output);
  delete output;
}
} // namespace ura
