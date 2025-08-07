#include "ura/toplevel.hpp"
#include <wayland-server-core.h>
#include <utility>
#include "ura/runtime.hpp"
#include "ura/server.hpp"
#include "ura/output.hpp"
#include "ura/ura.hpp"
#include "ura/callback.hpp"
#include "ura/seat.hpp"
#include "ura/util.hpp"
#include "ura/lua.hpp"

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
  this->workspace->add(this);
  xdg_toplevel->base->surface->data = this;
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
  this->destroying = true;
  if (this->is_active()) {
    server->seat->unfocus();
    workspace->remove(this);
    auto top = workspace->focus_stack.top();
    if (top)
      server->seat->focus(top.value());
  } else {
    workspace->remove(this);
  }
  server->runtime->remove(this);
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
    server->seat->focus(this);
    server->lua->try_execute_hook("window-new");
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
  if (this->resize(mode.width, mode.height))
    changed = true;
  if (this->move(mode.x, mode.y))
    changed = true;
  return changed;
}

bool UraToplevel::commit_floating() {
  // only commit floating state once
  if (this->fullscreen() || !this->floating || !this->initial_floating_commit)
    return false;
  this->initial_floating_commit = false;
  this->set_layer(this->output->floating);
  auto server = UraServer::get_instance();
  this->resize(
    server->lua->fetch<int>("layout.floating.default.width").value_or(800),
    server->lua->fetch<int>("layout.floating.default.height").value_or(600)
  );
  this->center();
  return true;
}

bool UraToplevel::commit_normal() {
  if (!this->is_normal())
    return false;
  this->set_layer(this->output->normal);

  int x, y, w, h;

  auto server = UraServer::get_instance();
  auto obj = server->lua->try_execute_hook("tiling");
  if (obj) {
    auto result = obj->as<std::optional<sol::table>>();
    if (!result)
      goto FALLBACK;
    auto tx = result.value().get<std::optional<int>>("x");
    auto ty = result.value().get<std::optional<int>>("y");
    auto tw = result.value().get<std::optional<int>>("width");
    auto th = result.value().get<std::optional<int>>("height");
    if (!tx || !ty || !tw || !th)
      goto FALLBACK;
    x = tx.value();
    y = ty.value();
    w = tw.value();
    h = th.value();
  } else {
  FALLBACK:
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
    auto inner =
      server->lua->fetch<int>("layout.tilling.gap.inner").value_or(10);
    auto& toplevels = output->current_workspace->toplevels;
    // find mapped toplevel number
    int sum = 0;
    for (auto toplevel : toplevels) {
      if (toplevel->is_normal())
        sum += 1;
    }
    // no toplevel to arrage
    if (sum == 0)
      return false;
    // find this toplevel's index
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
    w = (width - (outer_r + outer_l) - inner * gaps) / sum;
    h = height - (outer_t + outer_b);
    x = usable_area.x + outer_l + (w + inner) * i;
    y = usable_area.y + outer_t;
  }

  auto changed = false;
  if (this->resize(w, h))
    changed = true;
  if (this->move(x, y))
    changed = true;
  return changed;
}

void UraToplevel::focus() {
  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto surface = this->xdg_toplevel->base->surface;
  auto workspace = this->workspace;

  if (this->is_active())
    return;

  // make sure this is on stack
  if (!workspace->focus_stack.contains(this))
    workspace->focus_stack.push(this);

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
  if (!this->destroying)
    wlr_xdg_toplevel_set_activated(this->xdg_toplevel, false);
  auto server = UraServer::get_instance();
  server->seat->text_input->unfocus_active_text_input();
}

// get toplevel instance from wlr_surface
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

void UraToplevel::move_to_scratchpad() {
  auto server = UraServer::get_instance();
  if (this->is_active())
    server->seat->unfocus();
  this->unmap();
  auto scratchpad = server->scratchpad.get();
  this->workspace->toplevels.remove(this);
  this->workspace->focus_stack.remove(this);
  auto prev_workspace = this->workspace;
  this->workspace = scratchpad;
  this->workspace->toplevels.push_back(this);
  for (auto toplevel : prev_workspace->toplevels) toplevel->request_commit();
  if (prev_workspace->focus_stack.size()) {
    server->seat->focus(prev_workspace->focus_stack.top().value());
  }
}

std::optional<int> UraToplevel::move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->current_output();
  auto target = output->get_workspace_at(index);
  if (!target)
    return {};
  if (target == this->workspace)
    return index;
  // switch focus
  if (this->is_active())
    server->seat->unfocus();
  this->workspace->remove(this);
  if (this->workspace->focus_stack.top())
    server->seat->focus(this->workspace->focus_stack.top().value());
  this->workspace = target;
  this->workspace->add(this);
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
  server->seat->focus(this);
  server->lua->try_execute_hook("activate");
}

bool UraToplevel::move(int x, int y, bool force_update_border) {
  if (x != this->geometry.x && y != this->geometry.y) {
    this->geometry.x = x;
    this->geometry.y = y;
    wlr_scene_node_set_position(
      &this->scene_tree->node,
      this->geometry.x,
      this->geometry.y
    );
  }
  if (x == this->geometry.x && y == this->geometry.y && !force_update_border)
    return false;
  auto border_width = this->border_width;
  // top border
  wlr_scene_node_set_position(
    &this->borders[0]->node,
    -border_width,
    -border_width
  );
  // right border
  wlr_scene_node_set_position(
    &this->borders[1]->node,
    this->geometry.width,
    -border_width
  );
  // bottom border
  wlr_scene_node_set_position(
    &this->borders[2]->node,
    -border_width,
    this->geometry.height
  );
  // left border
  wlr_scene_node_set_position(
    &this->borders[3]->node,
    -border_width,
    -border_width
  );
  return true;
}

bool UraToplevel::resize(int width, int height) {
  if (width <= 0)
    width = 1;
  if (height <= 0)
    height = 1;
  if (width == this->geometry.width && height == this->geometry.height)
    return false;
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
  this->move(this->geometry.x, this->geometry.y, true);
  return true;
}

void UraToplevel::set_fullscreen(bool flag) {
  if (this->xdg_toplevel->base->initialized) {
    wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
    if (this->floating)
      this->initial_floating_commit = true;
  }
}

bool UraToplevel::fullscreen() {
  if (!this->xdg_toplevel)
    return false;
  return this->xdg_toplevel->current.fullscreen;
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
  return this->xdg_toplevel->title ? this->xdg_toplevel->title : "";
}

std::string UraToplevel::app_id() {
  return this->xdg_toplevel->app_id ? this->xdg_toplevel->app_id : "";
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
    this->initial_floating_commit = true;
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

void UraToplevel::create_borders() {
  auto server = UraServer::get_instance();
  auto active_border_color =
    server->lua->fetch<std::string>("opt.active_border_color")
      .value_or("#89b4fa");
  auto inactive_border_color =
    server->lua->fetch<std::string>("opt.inactive_border_color")
      .value_or("#00000000");

  this->border_width = server->lua->fetch<uint>("opt.border_width").value_or(1);
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
  auto server = UraServer::get_instance();
  return server->seat->seat->keyboard_state.focused_surface
    == this->xdg_toplevel->base->surface;
}

void UraToplevel::set_border_color(std::array<float, 4>& color) {
  for (auto border : this->borders) {
    wlr_scene_rect_set_color(border, color.data());
  }
}

// move to center of usable area
void UraToplevel::center() {
  auto geo = this->geometry;
  auto usable_area = this->output->usable_area;
  auto x = usable_area.x + (usable_area.width - geo.width) / 2;
  auto y = usable_area.y + (usable_area.height - geo.height) / 2;
  this->move(x, y);
}

sol::table UraToplevel::to_lua_table() {
  auto server = UraServer::get_instance();
  auto table = server->lua->state.create_table();
  table["index"] = this->index();
  table["workspace_index"] =
    this->workspace != server->scratchpad.get() ? this->workspace->index() : -1;
  table["app_id"] = this->app_id();
  table["title"] = this->title();
  table["floating"] = this->floating;
  table["fullscreen"] = this->fullscreen();
  table["x"] = this->geometry.x;
  table["y"] = this->geometry.y;
  table["width"] = this->geometry.width;
  table["height"] = this->geometry.height;
  return table;
}
} // namespace ura
