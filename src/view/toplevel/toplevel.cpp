#include "ura/view/toplevel.hpp"
#include "flexible/flexible.hpp"
#include "ura/ura.hpp"
#include "ura/util/vec.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/core/callback.hpp"
#include "ura/seat/seat.hpp"
#include "ura/util/util.hpp"
#include "ura/core/state.hpp"
#include "ura/view/view.hpp"

namespace ura {

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  this->xdg_toplevel = xdg_toplevel;
  this->z_index = UraSceneLayer::Normal;
  this->scene_tree = wlr_scene_xdg_surface_create(
    server->view->get_scene_tree_or_create(this->z_index),
    xdg_toplevel->base
  );
  this->output = output->name;
  this->workspace = output->current_workspace;
  this->workspace->add(this);
  xdg_toplevel->base->surface->data = this;
  this->create_borders();

  server->view->notify_scale(
    this->xdg_toplevel->base->surface,
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
    server->runtime->register_callback(
      &xdg_toplevel->events.set_app_id,
      on_toplevel_set_app_id,
      this
    );
    server->runtime->register_callback(
      &xdg_toplevel->events.set_title,
      on_toplevel_set_title,
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

  server->globals[this->id()] = UraGlobalType::Toplevel;
}

void UraToplevel::destroy() {
  auto server = UraServer::get_instance();
  this->destroying = true;
  if (this->is_focused()) {
    server->seat->unfocus();
    this->workspace->remove(this);
    auto top = this->workspace->focus_stack.top();
    if (top)
      server->seat->focus(top.value());
  } else {
    this->workspace->remove(this);
  }
  server->runtime->remove(this);
  wlr_foreign_toplevel_handle_v1_destroy(this->foreign_handle);
  this->dismiss_popups();

  server->globals.erase(this->id());
}

void UraToplevel::commit() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  if (!output)
    return;
  if (!this->xdg_toplevel->base->initialized) {
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
      output->output
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
    wlr_xdg_toplevel_set_size(this->xdg_toplevel, 0, 0);
    return;
  }

  // second commit
  if (!this->prepared) {
    auto geo = Vec4<int>::from(this->xdg_toplevel->base->geometry);
    geo.center(output->usable_area);
    this->geometry.width = geo.width;
    this->geometry.height = geo.height;
    this->resize_borders(geo.width, geo.height);
    this->move(geo.x, geo.y);

    server->seat->focus(this);

    auto args = flexible::create_table();
    args.set("id", this->id());
    server->state->try_execute_hook("window-new", args);

    this->prepared = true;
    this->map();
  }
}

void UraToplevel::focus() {
  if (!this->xdg_toplevel->base->initialized)
    return;
  if (this->is_focused())
    return;

  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto surface = this->xdg_toplevel->base->surface;
  auto workspace = this->workspace;

  // make sure this is on stack
  if (!workspace->focus_stack.contains(this))
    workspace->focus_stack.push(this);

  // move to top of stack and focus this
  workspace->focus_stack.move_to_top(this);
  wlr_scene_node_raise_to_top(&this->scene_tree->node);
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
  this->set_border_color(this->active_border_color);
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
  server->seat->text_input->focus_text_input(surface);
}

void UraToplevel::unfocus() {
  if (!this->xdg_toplevel->base->initialized)
    return;
  if (!this->is_focused())
    return;

  this->set_border_color(this->inactive_border_color);

  if (!this->destroying) {
    wlr_xdg_toplevel_set_activated(this->xdg_toplevel, false);
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
  }

  auto server = UraServer::get_instance();
  server->seat->text_input->unfocus_active_text_input();

  this->dismiss_popups();
}

// get toplevel instance from wlr_surface, it asserts the surface's role is xdg_toplevel
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

UraToplevel* UraToplevel::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (server->globals.contains(id)
      && server->globals[id].type == UraGlobalType::Toplevel)
    return reinterpret_cast<UraToplevel*>(id);
  return nullptr;
}

void UraToplevel::move_to_workspace(UraWorkspace* workspace) {
  workspace->name ? this->move_to_workspace(workspace->name.value())
                  : this->move_to_workspace(workspace->index());
}

void UraToplevel::move_to_workspace(std::string name) {
  auto server = UraServer::get_instance();
  if (this->is_focused())
    server->seat->unfocus();
  this->unmap();
  auto named_workspace = server->view->get_named_workspace_or_create(name);
  this->workspace->remove(this);
  auto prev_workspace = this->workspace;
  this->workspace = named_workspace;
  this->workspace->toplevels.push_back(this);
  if (prev_workspace->focus_stack.top()) {
    server->seat->focus(prev_workspace->focus_stack.top().value());
  }
}

void UraToplevel::move_to_workspace(int index) {
  auto server = UraServer::get_instance();
  auto output = server->view->current_output();
  auto target = output->get_workspace_at(index);
  if (!target)
    return;
  if (target == this->workspace)
    return;
  // switch focus
  if (this->is_focused())
    server->seat->unfocus();
  this->workspace->remove(this);
  if (this->workspace->focus_stack.top())
    server->seat->focus(this->workspace->focus_stack.top().value());
  this->workspace = target;
  this->workspace->add(this);
}

int UraToplevel::index() {
  auto server = UraServer::get_instance();
  int index = 0;
  auto& toplevels = this->workspace->toplevels;
  for (auto toplevel : toplevels) {
    if (toplevel == this)
      return index;
    index++;
  }
  std::unreachable();
}

void UraToplevel::activate() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  auto args = flexible::create_table();
  args.set("id", this->id());
  auto flag =
    server->state->try_execute_hook<bool>("pre-window-activate", args);
  // if hook returns a false value, then stop the operation.
  if (flag && !flag.value())
    return;
  if (this->workspace->name) {
    // named workspace, move this toplevel to current workspace
    this->move_to_workspace(output->current_workspace);
  } else if (this->workspace != output->current_workspace) {
    // indexed workspace, switch to this toplevel's workspace
    output->switch_workspace(this->workspace);
  }
  server->seat->focus(this);
  this->map();
  server->state->try_execute_hook("post-window-activate", args);
}

bool UraToplevel::move(int x, int y) {
  this->move_borders(x, y);

  if (x != this->geometry.x || y != this->geometry.y) {
    this->geometry.x = x;
    this->geometry.y = y;
    wlr_scene_node_set_position(
      &this->scene_tree->node,
      this->geometry.x,
      this->geometry.y
    );

    auto server = UraServer::get_instance();
    auto args = flexible::create_table();
    args.set("id", this->id());
    server->state->try_execute_hook("window-move", args);

    return true;
  }

  return false;
}

bool UraToplevel::resize(int width, int height) {
  if (width <= 0 || height <= 0)
    return false;
  if (width == this->geometry.width && height == this->geometry.height)
    return false;
  this->geometry.width = width;
  this->geometry.height = height;
  wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
  this->resize_borders(width, height);
  this->move(this->geometry.x, this->geometry.y);

  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->state->try_execute_hook("window-resize", args);
  return true;
}

void UraToplevel::resize_borders(int width, int height) {
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

void UraToplevel::close() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  wlr_foreign_toplevel_handle_v1_output_leave(
    this->foreign_handle,
    output->output
  );
}

void UraToplevel::map() {
  if (this->mapped)
    return;
  this->mapped = true;
  wlr_scene_node_set_enabled(&this->scene_tree->node, true);
  if (this->xdg_toplevel->base->initialized && this->foreign_handle) {
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
    wlr_foreign_toplevel_handle_v1_set_title(
      this->foreign_handle,
      this->title().data()
    );
    wlr_foreign_toplevel_handle_v1_set_app_id(
      this->foreign_handle,
      this->app_id().data()
    );
  }
}

void UraToplevel::unmap() {
  if (!this->mapped)
    return;
  this->mapped = false;
  wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  if (this->xdg_toplevel->base->initialized && this->foreign_handle)
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
}

std::string UraToplevel::title() {
  return this->xdg_toplevel->title ? this->xdg_toplevel->title : "";
}

std::string UraToplevel::app_id() {
  return this->xdg_toplevel->app_id ? this->xdg_toplevel->app_id : "";
}

void UraToplevel::set_title(std::string title) {
  wlr_foreign_toplevel_handle_v1_set_title(this->foreign_handle, title.data());
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->state->try_execute_hook("window-title-change", args);
}

void UraToplevel::set_app_id(std::string app_id) {
  wlr_foreign_toplevel_handle_v1_set_app_id(
    this->foreign_handle,
    app_id.data()
  );
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->state->try_execute_hook("window-app_id-change", args);
}

void UraToplevel::set_z_index(int z_index) {
  auto server = UraServer::get_instance();
  if (this->z_index != z_index) {
    auto layer = server->view->get_scene_tree_or_create(z_index);
    wlr_scene_node_reparent(&this->scene_tree->node, layer);
    this->z_index = z_index;
  }
}

void UraToplevel::create_borders() {
  auto server = UraServer::get_instance();
  auto active_border_color =
    server->state->get_option<std::string>("active_border_color")
      .value_or("#89b4fa");
  auto inactive_border_color =
    server->state->get_option<std::string>("inactive_border_color")
      .value_or("#00000000");
  this->border_width =
    server->state->get_option<int>("border_width").value_or(1);
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

bool UraToplevel::is_focused() {
  auto server = UraServer::get_instance();
  return server->seat->focused_toplevel() == this;
}

void UraToplevel::set_border_color(std::array<float, 4>& color) {
  for (auto border : this->borders) {
    wlr_scene_rect_set_color(border, color.data());
  }
}

void UraToplevel::center() {
  auto server = UraServer::get_instance();
  auto output = server->view->get_output_by_name(this->output);
  auto area = output->logical_geometry();
  auto geo = this->geometry;
  geo.center(area);
  this->move(geo.x, geo.y);
}

void UraToplevel::dismiss_popups() {
  wlr_xdg_popup *_popup, *tmp;
  wl_list_for_each_safe(_popup, tmp, &this->xdg_toplevel->base->popups, link) {
    wlr_xdg_popup_destroy(_popup);
  }
}

uint64_t UraToplevel::id() {
  return reinterpret_cast<uint64_t>(this);
}

void UraToplevel::set_fullscreen(bool flag) {
  wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
  if (this->foreign_handle)
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
}

bool UraToplevel::is_fullscreen() {
  return this->xdg_toplevel->current.fullscreen;
}

void UraToplevel::move_borders(int x, int y) {
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
}
} // namespace ura
