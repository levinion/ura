#include "ura/toplevel.hpp"
#include <wayland-server-core.h>
#include <utility>
#include "ura/client.hpp"
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"
#include "ura/seat.hpp"
#include "ura/util.hpp"

namespace ura {

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  // setup ura toplevel
  this->xdg_toplevel = xdg_toplevel;
  // add to output's normal layer
  this->scene_tree =
    wlr_scene_xdg_surface_create(output->normal, xdg_toplevel->base);
  this->layer = output->normal;
  this->output = output;
  this->workspace = this->output->current_workspace;
  this->workspace->toplevels.push_back(this);
  xdg_toplevel->base->surface->data = this;
  this->floating_width =
    server->lua->fetch<int>("layout.floating.default.width").value_or(800);
  this->floating_height =
    server->lua->fetch<int>("layout.floating.default.height").value_or(600);
  this->create_borders();

  // notify scale
  wlr_fractional_scale_v1_notify_scale(
    xdg_toplevel->base->surface,
    output->output->scale
  );
  wlr_surface_set_preferred_buffer_scale(
    xdg_toplevel->base->surface,
    output->output->scale
  );

  // register callback
  {
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.map,
      on_toplevel_map,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.unmap,
      on_toplevel_unmap,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->base->surface->events.commit,
      on_toplevel_commit,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.destroy,
      on_toplevel_destroy,
      this
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
    // server->runtime->register_callback(
    //   &xdg_toplevel->events.request_maximize,
    //   on_toplevel_request_maximize,
    //   this
    // );
    server->runtime->register_callback(
      &xdg_toplevel->events.request_fullscreen,
      on_toplevel_request_fullscreen,
      this
    );
  }

  // forign toplevel handle
  this->foreign_handle =
    wlr_foreign_toplevel_handle_v1_create(server->foreign_manager);
  this->foreign_handle->data = this;

  server->runtime->register_callback(
    &this->foreign_handle->events.request_fullscreen,
    on_foreign_toplevel_handle_request_fullscreen,
    this
  );
  server->runtime->register_callback(
    &this->foreign_handle->events.request_activate,
    on_foreign_toplevel_handle_request_activate,
    this
  );
}

void UraToplevel::destroy() {
  auto server = UraServer::get_instance();
  auto workspace = this->workspace;
  auto is_top = workspace->focus_stack.is_top(this);
  workspace->focus_stack.remove(this);
  auto top = workspace->focus_stack.top();
  if (is_top && top) {
    top.value().focus();
  }
  server->runtime->remove(this);
  workspace->toplevels.remove(this);
  wlr_foreign_toplevel_handle_v1_destroy(this->foreign_handle);
}

void UraToplevel::commit() {
  auto server = UraServer::get_instance();
  if (!this->mapped || !this->xdg_toplevel->base->initialized) {
    return;
  }
  // first commit
  if (this->xdg_toplevel->base->initial_commit) {
    if (this->decoration)
      wlr_xdg_toplevel_decoration_v1_set_mode(
        this->decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
      );
    wlr_foreign_toplevel_handle_v1_output_enter(
      this->foreign_handle,
      this->output->output
    );
    if (this->xdg_toplevel->title)
      wlr_foreign_toplevel_handle_v1_set_title(
        this->foreign_handle,
        this->xdg_toplevel->title
      );
    if (this->xdg_toplevel->app_id)
      wlr_foreign_toplevel_handle_v1_set_app_id(
        this->foreign_handle,
        this->xdg_toplevel->app_id
      );
    this->focus();
  }

  if (this->commit_fullscreen() || this->commit_floating()
      || this->commit_normal()) {
    for (auto toplevel : this->workspace->toplevels)
      if (toplevel != this)
        toplevel->request_commit();
  }
}

// handle fullscreen toplevel window
bool UraToplevel::commit_fullscreen() {
  if (!this->fullscreen())
    return false;
  this->set_layer(this->output->fullscreen);
  auto changed = false;
  auto mode = this->output->logical_geometry();
  auto geo = this->geometry;
  if (geo.width != mode.width || geo.height != mode.height) {
    this->resize(mode.width, mode.height);
    changed = true;
  }
  if (geo.x != mode.x || geo.y != mode.y) {
    this->move(mode.x, mode.y);
    changed = true;
  }
  return changed;
}

bool UraToplevel::commit_floating() {
  if (!this->floating)
    return false;
  this->set_layer(this->output->floating);
  auto changed = false;
  auto geo = this->geometry;
  auto usable_area = this->output->usable_area;
  auto sx = usable_area.x;
  auto sw = usable_area.width;
  auto sy = usable_area.y;
  auto sh = usable_area.height;
  auto tw = this->floating_width;
  auto th = this->floating_height;
  if (geo.width != tw || geo.height != th) {
    this->resize(tw, th);
    changed = true;
  }
  auto x = sx + (sw - tw) / 2;
  auto y = sy + (sh - th) / 2;
  if (geo.x != x || geo.y != y) {
    this->move(x, y);
    changed = true;
  }
  return changed;
}

bool UraToplevel::commit_normal() {
  if (!this->is_normal())
    return false;
  this->set_layer(this->output->normal);
  auto changed = false;
  auto server = UraServer::get_instance();
  auto geo = this->geometry;
  auto usable_area = this->output->usable_area;
  auto width = usable_area.width;
  auto height = usable_area.height;
  auto outer_l =
    server->lua->fetch<int>("layout.tilling.gap.outer.left").value_or(10);
  auto outer_r =
    server->lua->fetch<int>("layout.tilling.gap.outer.right").value_or(10);
  auto outer_t =
    server->lua->fetch<int>("layout.tilling.gap.outer.top").value_or(10);
  auto outer_b =
    server->lua->fetch<int>("layout.tilling.gap.outer.bottom").value_or(10);
  auto inner = server->lua->fetch<int>("layout.tilling.gap.inner").value_or(10);
  auto& toplevels = output->current_workspace->toplevels;
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
    if (window != this)
      i++;
    else
      break;
  }
  auto gaps = sum - 1;
  auto w = (width - (outer_r + outer_l) - inner * gaps) / sum;
  auto h = height - (outer_t + outer_b);
  auto x = usable_area.x + outer_l + (w + inner) * i;
  auto y = usable_area.y + outer_t;

  if (geo.width != w || geo.height != h) {
    this->resize(w, h);
    this->move(x, y);
    changed = true;
  }
  if (geo.x != x || geo.y != y) {
    this->move(x, y);
    changed = true;
  }
  return changed;
}

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto surface = this->xdg_toplevel->base->surface;
  auto workspace = this->workspace;
  // if not on top of focus stack, then unfocus the current top
  if (workspace->focus_stack.size() != 0
      && !workspace->focus_stack.is_top(this)) {
    auto prev = workspace->focus_stack.top().value();
    if (prev.type == UraSurfaceType::Toplevel) {
      auto toplevel = prev.transform<UraToplevel>();
      toplevel->unfocus();
    }
  }
  // move to top of stack and focus this
  workspace->focus_stack.move_to_top(this);
  if (!this->mapped)
    this->map();
  wlr_scene_node_raise_to_top(&this->scene_tree->node);
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
  this->set_border_color(this->active_border_color);
  server->seat->text_input->focus_text_input(surface);
  auto keyboard = wlr_seat_get_keyboard(seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      seat,
      surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}

void UraToplevel::unfocus() {
  this->set_border_color(this->inactive_border_color);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, false);
}

// get toplevel instance from wlr_surface
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

int UraToplevel::move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto target = output->get_workspace_at(index);
  if (!target)
    return -1;
  if (this->workspace) {
    this->workspace->toplevels.remove(this);
    this->workspace->focus_stack.remove(this);
  }
  this->workspace = target;
  this->workspace->toplevels.push_back(this);
  this->workspace->focus_stack.push(this);
  return this->workspace->index();
}

int UraToplevel::index() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  int index = 0;
  for (auto toplevel : this->workspace->toplevels) {
    if (toplevel == this)
      return index;
    index++;
  }
  std::unreachable();
}

void UraToplevel::activate() {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto current_workspace = output->current_workspace->index();
  if (server->scratchpad.get() == this->workspace) {
    this->move_to_workspace(current_workspace);
  } else if (this->workspace->index() != current_workspace) {
    output->switch_workspace(this->workspace);
  }
  this->focus();
  server->lua->try_execute_hook("activate");
}

void UraToplevel::move(int x, int y) {
  this->geometry.x = x;
  this->geometry.y = y;
  auto border_width = this->border_width;
  auto geo = this->geometry;
  wlr_scene_node_set_position(&this->scene_tree->node, geo.x, geo.y);
  // top border
  wlr_scene_node_set_position(
    &this->borders[0]->node,
    -border_width,
    -border_width
  );
  // right border
  wlr_scene_node_set_position(
    &this->borders[1]->node,
    geo.width,
    -border_width
  );
  // bottom border
  wlr_scene_node_set_position(
    &this->borders[2]->node,
    -border_width,
    geo.height
  );
  // left border
  wlr_scene_node_set_position(
    &this->borders[3]->node,
    -border_width,
    -border_width
  );
}

void UraToplevel::resize(int width, int height) {
  this->geometry.width = width;
  this->geometry.height = height;
  wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
  auto border_width = this->border_width;
  // top border
  wlr_scene_rect_set_size(
    this->borders[0],
    width + 2 * border_width,
    border_width
  );
  // right border
  wlr_scene_rect_set_size(
    this->borders[1],
    border_width,
    height + 2 * border_width
  );
  // bottom border
  wlr_scene_rect_set_size(
    this->borders[2],
    width + 2 * border_width,
    border_width
  );
  // left border
  wlr_scene_rect_set_size(
    this->borders[3],
    border_width,
    height + 2 * border_width
  );
}

void UraToplevel::set_fullscreen(bool flag) {
  if (this->xdg_toplevel->base->initialized) {
    wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
  }
}

bool UraToplevel::fullscreen() {
  if (!this->xdg_toplevel)
    return false;
  return this->xdg_toplevel->pending.fullscreen;
}

void UraToplevel::toggle_fullscreen() {
  this->set_fullscreen(!this->fullscreen());
}

void UraToplevel::close() {
  wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  wlr_foreign_toplevel_handle_v1_output_leave(
    this->foreign_handle,
    this->output->output
  );
}

void UraToplevel::map() {
  this->mapped = true;
  wlr_scene_node_set_enabled(&this->scene_tree->node, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
  wlr_foreign_toplevel_handle_v1_set_title(
    this->foreign_handle,
    this->xdg_toplevel->title
  );
  wlr_foreign_toplevel_handle_v1_set_app_id(
    this->foreign_handle,
    this->xdg_toplevel->app_id
  );
}

void UraToplevel::unmap() {
  this->mapped = false;
  wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
}

std::string UraToplevel::title() {
  return this->xdg_toplevel->title;
}

void UraToplevel::set_title(std::string title) {
  this->xdg_toplevel->title = title.data();
  wlr_foreign_toplevel_handle_v1_set_title(this->foreign_handle, title.data());
}

bool UraToplevel::is_normal() {
  return (this->mapped && !this->fullscreen() && !this->floating);
}

void UraToplevel::set_float(bool flag) {
  if (flag && !this->floating) {
    this->floating = true;
    wlr_scene_node_reparent(&this->scene_tree->node, this->output->floating);
    return;
  }
  if (!flag && this->floating) {
    this->floating = false;
    wlr_scene_node_reparent(&this->scene_tree->node, this->output->normal);
  }
}

void UraToplevel::set_layer(wlr_scene_tree* layer) {
  if (this->layer != layer) {
    wlr_scene_node_reparent(&this->scene_tree->node, layer);
    this->layer = layer;
  }
}

// request the toplevel to send a commit request now
void UraToplevel::request_commit() {
  wlr_xdg_surface_schedule_configure(this->xdg_toplevel->base);
}

void UraToplevel::move_to_scratchpad() {
  auto server = UraServer::get_instance();
  auto& scratchpad = server->scratchpad;
  this->workspace->toplevels.remove(this);
  this->workspace->focus_stack.remove(this);
  auto prev_workspace = this->workspace;
  this->workspace = scratchpad.get();
  this->workspace->toplevels.push_back(this);
  for (auto toplevel : prev_workspace->toplevels) toplevel->request_commit();
}

void UraToplevel::create_borders() {
  auto server = UraServer::get_instance();
  auto active_border_color =
    server->lua->fetch<std::string>("win.border.active_color")
      .value_or("#89b4fa");
  auto inactive_border_color =
    server->lua->fetch<std::string>("win.border.inactive_color")
      .value_or("#00000000");

  this->border_width = server->lua->fetch<uint>("win.border.width").value_or(1);
  this->active_border_color =
    hex2rgba(active_border_color)
      .value_or({ 137.f / 255.f, 180.f / 255.f, 250.f / 255.f, 1.f });
  this->inactive_border_color =
    hex2rgba(inactive_border_color).value_or({ 0.f, 0.f, 0.f, 0.f });

  for (int i = 0; i < 4; i++) {
    this->borders[i] = wlr_scene_rect_create(
      this->scene_tree,
      0,
      0,
      this->active_border_color.data()
    );
  }
}

bool UraToplevel::is_active() {
  return this->xdg_toplevel->current.activated;
}

void UraToplevel::set_border_color(std::array<float, 4>& color) {
  for (auto border : this->borders) {
    wlr_scene_rect_set_color(border, color.data());
  }
}
} // namespace ura
