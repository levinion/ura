#include "ura/server.hpp"
#include "ura/output.hpp"
#include <cassert>
#include "ura/callback.hpp"
#include "ura/ura.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();

  // setup wlr_output
  auto _wlr_output = static_cast<wlr_output*>(data);
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  auto mode = wlr_output_preferred_mode(_wlr_output);
  if (mode) {
    wlr_output_state_set_mode(&state, mode);
  }

  wlr_output_commit_state(_wlr_output, &state);
  wlr_output_state_finish(&state);

  // create ura output object from _wlr_output
  auto output = new UraOutput {};
  output->output = _wlr_output;

  // register callback
  server->runtime
    ->register_callback(&_wlr_output->events.frame, on_output_frame, output);

  server->runtime->register_callback(
    &_wlr_output->events.request_state,
    on_output_request_state,
    output
  );

  server->runtime->register_callback(
    &_wlr_output->events.destroy,
    on_output_destroy,
    output
  );

  // add this wlr_output to scene layout
  auto output_layout_output =
    wlr_output_layout_add_auto(server->output_layout, _wlr_output);
  auto scene_output = wlr_scene_output_create(server->scene, _wlr_output);
  wlr_scene_output_layout_add_output(
    server->scene_layout,
    output_layout_output,
    scene_output
  );
}

void on_output_frame(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->runtime->fetch<UraOutput*>(listener);

  auto scene = server->scene;
  auto scene_output = wlr_scene_get_scene_output(scene, output->output);
  wlr_scene_output_commit(scene_output, nullptr);

  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
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
}

} // namespace ura
