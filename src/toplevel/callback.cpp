
#include "ura/toplevel.hpp"
#include "ura/runtime.hpp"
#include "ura/output.hpp"
#include "ura/server.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"
#include "ura/workspace.hpp"

namespace ura {
// create a new toplevel
void on_new_toplevel(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);
  auto toplevel = new UraToplevel {};
  toplevel->init(xdg_toplevel);
}

void on_toplevel_map(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  if (!toplevel->mapped)
    toplevel->map();
  toplevel->focus();
}

void on_toplevel_unmap(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->unmap();
}

void on_toplevel_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  auto output = server->current_output();

  if (!output || !toplevel->mapped || !toplevel->initialized()) {
    return;
  }

  auto scale = server->current_output()->output->scale;
  auto mode = output->output->current_mode;

  // handle fullscreen toplevel window
  if (toplevel->fullscreen()) {
    toplevel->focus();
    toplevel->resize(mode->width / scale, mode->height / scale);
    toplevel->move(0, 0);
    return;
  }

  if (toplevel->xdg_toplevel->base->initial_commit && toplevel->decoration) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      toplevel->decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }

  output->configure_layers();
  auto usable_area = output->usable_area;
  // auto width = mode->width / scale;
  // auto height = mode->height / scale;
  auto width = usable_area.width;
  auto height = usable_area.height;

  auto outer = server->config->outer_gap;
  auto inner = server->config->inner_gap;
  auto& toplevels = server->current_output()->current_workspace->toplevels;
  // find mapped toplevel number
  int sum = 0;
  for (auto toplevel : toplevels) {
    if (toplevel->is_normal())
      sum += 1;
  }
  // find this toplevel index
  int i = 0;
  for (auto window : toplevels) {
    if (!window->is_normal())
      continue;
    if (window != toplevel)
      i++;
    else
      break;
  }
  auto gaps = sum - 1;
  auto w = (width - 2 * outer - inner * gaps) / sum;
  auto h = height - 2 * outer;
  auto x = usable_area.x + outer + (w + inner) * i;
  auto y = usable_area.y + outer;
  // check value
  if (w < 0 || h < 0 || w > width || h > height)
    return;
  toplevel->resize(w, h);
  toplevel->move(x, y);
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  auto workspace = toplevel->workspace;

  // destroy this toplevel
  server->runtime->remove(toplevel);
  toplevel->output->current_workspace->toplevels.remove(toplevel);
  delete toplevel;

  /* switch focus to another toplevel */
  // if prev focused toplevel exists and in the same workspace, focus it
  if (server->prev_focused_toplevel
      && server->prev_focused_toplevel->workspace == workspace) {
    server->prev_focused_toplevel->focus();
    server->prev_focused_toplevel = nullptr;
  } else if (!output->current_workspace->toplevels.empty()) {
    // else focus a toplevel in the same workspace if any
    output->current_workspace->toplevels.back()->focus();
  } else {
    // else let it empty
    server->focused_toplevel = nullptr;
  }
}

// void on_toplevel_request_move(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   toplevel->move();
// }
//
// void on_toplevel_request_resize(wl_listener* listener, void* data) {
//   auto server = UraServer::get_instance();
//   auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
//   auto event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
//   toplevel->resize(event->edges);
// }

void on_toplevel_request_maximize(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  // equal to fullscreen
  toplevel->toggle_fullscreen();
}

void on_toplevel_request_fullscreen(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  toplevel->toggle_fullscreen();
}
} // namespace ura
