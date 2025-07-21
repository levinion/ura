#include "ura/callback.hpp"
#include "ura/server.hpp"
#include "ura/runtime.hpp"
#include "ura/output.hpp"

namespace ura {
void on_activation_request_activate(wl_listener* listener, void* data) {
  auto event = static_cast<wlr_xdg_activation_v1_request_activate_event*>(data);
  auto server = UraServer::get_instance();
  auto surface = event->surface;
  for (auto output : server->runtime->outputs)
    for (auto& workspace : output->workspaces)
      for (auto toplevel : workspace->toplevels) {
        if (toplevel->xdg_toplevel->base->surface == surface) {
          auto current_workspace = server->current_output()->current_workspace;
          toplevel->move_to_workspace(current_workspace->index());
          toplevel->focus();
          output->commit_frame();
          return;
        }
      }
}
} // namespace ura
