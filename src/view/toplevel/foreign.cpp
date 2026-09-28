#include "ura/view/toplevel.hpp"
#include "ura/view/output.hpp"
#include "ura/core/server.hpp"
#include "ura/ura.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/lua.hpp"

namespace ura {
// toplevel/foreign.cpp
void on_foreign_toplevel_handle_request_activate(
  wl_listener* listener,
  void* data
) {
  auto event =
    static_cast<wlr_foreign_toplevel_handle_v1_activated_event*>(data);
  auto toplevel = static_cast<UraToplevel*>(event->toplevel->data);
  toplevel->activate();
}

void on_foreign_toplevel_handle_request_fullscreen(
  wl_listener* listener,
  void* data
) {
  auto event =
    static_cast<wlr_foreign_toplevel_handle_v1_fullscreen_event*>(data);
  auto server = UraServer::get_instance();
  auto toplevel = static_cast<UraToplevel*>(event->toplevel->data);

  auto args = flexible::create_table();
  args.set("id", toplevel->id());
  args.set("fullscreen", event->fullscreen);
  if (auto output = UraOutput::from(event->output)) {
    args.set("output_id", output->id());
    if (event->fullscreen)
      toplevel->set_fullscreen_output(output);
  }
  server->lua->emit_hook("window-request-fullscreen", args);
}
} // namespace ura
