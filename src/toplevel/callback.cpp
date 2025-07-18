
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

  // setup ura toplevel
  auto toplevel = new UraToplevel {};
  toplevel->xdg_toplevel = xdg_toplevel;
  toplevel->scene_tree = wlr_scene_xdg_surface_create(
    &UraServer::get_instance()->scene->tree,
    xdg_toplevel->base
  );
  toplevel->output = server->current_output();
  server->runtime->toplevels.push_back(toplevel);
  toplevel->output->current_workspace->add(toplevel);

  // this is needed to get top most toplevel
  toplevel->scene_tree->node.data = toplevel;
  // this is needed to create popup surface
  xdg_toplevel->base->data = toplevel->scene_tree;

  // // notify scale
  wlr_fractional_scale_v1_notify_scale(
    xdg_toplevel->base->surface,
    server->current_output()->output->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    xdg_toplevel->base->surface,
    server->current_output()->output->scale
  );

  // register callback
  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.map,
    on_toplevel_map,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.unmap,
    on_toplevel_unmap,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->base->surface->events.commit,
    on_toplevel_commit,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.destroy,
    on_toplevel_destroy,
    toplevel
  );

  // server->runtime->register_callback(
  //   &xdg_toplevel->events.request_move,
  //   on_toplevel_request_move,
  //   toplevel
  // );
  //
  // server->runtime->register_callback(
  //   &xdg_toplevel->events.request_resize,
  //   on_toplevel_request_resize,
  //   toplevel
  // );

  server->runtime->register_callback(
    &xdg_toplevel->events.request_maximize,
    on_toplevel_request_maximize,
    toplevel
  );

  server->runtime->register_callback(
    &xdg_toplevel->events.request_fullscreen,
    on_toplevel_request_fullscreen,
    toplevel
  );
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
  auto scale = server->current_output()->output->scale;
  auto mode = output->output->current_mode;
  auto width = mode->width / scale;
  auto height = mode->height / scale;

  if (!toplevel->mapped)
    return;

  // handle fullscreen toplevel window
  if (toplevel->fullscreen()) {
    toplevel->focus();
    toplevel->resize(width, height);
    toplevel->move(0, 0);
    return;
  }

  if (toplevel->xdg_toplevel->base->initial_commit && toplevel->decoration) {
    wlr_xdg_toplevel_decoration_v1_set_mode(
      toplevel->decoration,
      WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
    );
  }

  // else auto tiling
  auto outer = server->config->outer_gap;
  auto inner = server->config->inner_gap;
  auto& toplevels = server->current_output()->current_workspace->toplevels;
  // find mapped toplevel number
  int sum = 0;
  for (auto toplevel : toplevels) {
    if (toplevel->mapped && !toplevel->fullscreen())
      sum += 1;
  }
  // find this toplevel index
  int i = 0;
  for (auto window : toplevels) {
    if (!window->mapped || window->fullscreen())
      continue;
    if (window != toplevel)
      i++;
    else
      break;
  }
  auto gaps = sum - 1;
  auto w = (width - 2 * outer - inner * gaps) / sum;
  auto h = height - 2 * outer;
  auto x = outer + (w + inner) * i;
  auto y = outer;
  // check value
  if (w < 0 || h < 0 || w > width || h > height)
    return;
  toplevel->resize(w, h);
  toplevel->move(x, y);
}

void on_toplevel_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  server->focused_toplevel = nullptr;
  auto toplevel = server->runtime->fetch<UraToplevel*>(listener);
  server->runtime->toplevels.remove(toplevel);
  server->runtime->remove(toplevel);
  toplevel->output->current_workspace->toplevels.remove(toplevel);
  delete toplevel;

  if (!server->runtime->toplevels.empty()) {
    auto new_toplevel = server->runtime->toplevels.back();
    new_toplevel->focus();
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
