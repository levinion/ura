#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/callback.hpp"

namespace ura {

void on_new_output(wl_listener* listener, void* data) {
  UraServer* server = wl_container_of(listener, server, new_output);
  auto _wlr_output = static_cast<wlr_output*>(data);
  wlr_output_init_render(_wlr_output, server->allocator, server->renderer);

  wlr_output_state state;
  wlr_output_state_init(&state);
  wlr_output_state_set_enabled(&state, true);

  auto mode = wlr_output_preferred_mode(_wlr_output);
  if (mode) {
    // TODO: configure display mode here
    wlr_output_state_set_mode(&state, mode);
  }

  wlr_output_commit_state(_wlr_output, &state);
  wlr_output_state_finish(&state);

  auto output = new UraOutput {};
  output->output = _wlr_output;

  output->frame.notify = on_output_frame;
  wl_signal_add(&_wlr_output->events.frame, &output->frame);
  output->request_state.notify = on_output_request_state;
  wl_signal_add(&_wlr_output->events.request_state, &output->request_state);
  output->destroy.notify = on_output_destroy;
  wl_signal_add(&_wlr_output->events.destroy, &output->destroy);

  wl_list_insert(&server->outputs, &output->link);

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
  UraOutput* output = wl_container_of(listener, output, frame);
  auto scene = UraServer::get_instance()->scene;
  auto scene_output = wlr_scene_get_scene_output(scene, output->output);
  wlr_scene_output_commit(scene_output, nullptr);

  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}

void on_output_request_state(wl_listener* listener, void* data) {
  UraOutput* output = wl_container_of(listener, output, request_state);
  auto event = static_cast<wlr_output_event_request_state*>(data);
  wlr_output_commit_state(output->output, event->state);
}

void on_output_destroy(wl_listener* listener, void* data) {
  UraOutput* output = wl_container_of(listener, output, destroy);
  wl_list_remove(&output->frame.link);
  wl_list_remove(&output->request_state.link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->link);
  delete output;
}

} // namespace ura
