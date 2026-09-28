#include "ura/view/toplevel.hpp"
#include "ura/util/flexible.hpp"
#include "ura/ura.hpp"
#include "ura/util/rgb.hpp"
#include "ura/util/vec.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/core/callback.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/lua.hpp"
#include "ura/view/view.hpp"

namespace ura {

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  this->xdg_toplevel = xdg_toplevel;
  this->z_index = UraSceneLayer::Normal;
  this->scene_tree = wlr_scene_xdg_surface_create(
    server->view->get_scene_tree_or_create(this->z_index),
    xdg_toplevel->base
  );
  wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  auto output = server->view->current_output();
  if (output) {
    this->tags = output->tags;
  } else {
    this->tags = {};
  }
  xdg_toplevel->base->surface->data = this;
  this->set_scale(this->scale());

  server->view->toplevels.push_back(this);

  this->create_borders();

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
    //   &xdg_toplevel->events.request_resize,
    //   on_toplevel_request_resize,
    //   toplevel
    // );
    server->runtime->register_callback(
      &xdg_toplevel->events.request_maximize,
      on_toplevel_request_maximize,
      this
    );
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
  server->view->toplevels.remove(this);
  if (this->is_focused()) {
    server->seat->focus_lru();
  }
  server->runtime->remove(this);
  wlr_foreign_toplevel_handle_v1_destroy(this->foreign_handle);
  this->dismiss_popups();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-close", args);

  server->globals.erase(this->id());
}

void UraToplevel::commit() {
  auto server = UraServer::get_instance();
  UraOutput* output = nullptr;
  if (
    this->xdg_toplevel->requested.fullscreen
    && this->xdg_toplevel->requested.fullscreen_output
  ) {
    output = UraOutput::from(this->xdg_toplevel->requested.fullscreen_output);
  }
  if (!output)
    output = this->output();
  if (!output)
    output = server->view->current_output();
  if (!output || !this->xdg_toplevel->base->initialized) {
    return;
  }
  this->set_output(output);
  // first commit
  if (this->xdg_toplevel->base->initial_commit) {
    if (this->decoration)
      wlr_xdg_toplevel_decoration_v1_set_mode(
        this->decoration,
        WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
      );
    // let the client to decide its size
    if (
      this->xdg_toplevel->base->current.geometry.width == 0
      || this->xdg_toplevel->base->current.geometry.height == 0
    ) {
      wlr_xdg_toplevel_set_size(this->xdg_toplevel, 0, 0);
      return;
    }
  }

  // second commit
  if (!this->prepared) {
    // set a default size if the given size is invalid
    if (
      this->xdg_toplevel->base->current.geometry.width == 0
      || this->xdg_toplevel->base->current.geometry.height == 0
    ) {
      this->resize(800, 600);
    } else {
      this->geometry.width = this->xdg_toplevel->base->current.geometry.width;
      this->geometry.height = this->xdg_toplevel->base->current.geometry.height;
    }
    if (this->xdg_toplevel->requested.fullscreen) {
      auto geo = output->logical_geometry();
      this->resize(geo.width, geo.height);
      this->move(geo.x, geo.y);
    } else if (this->xdg_toplevel->requested.maximized) {
      auto geo = output->usable_area;
      this->resize(geo.width, geo.height);
      this->move(geo.x, geo.y);
    } else {
      this->center();
    }
    this->resize_borders(this->geometry.width, this->geometry.height);
    this->move_borders(this->geometry.x, this->geometry.y);
    this->prepared = true;
    this->apply_pending_fullscreen_output();

    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("window-new", args);
  }

  // update opacity
  this->set_opacity(this->opacity);
}

void UraToplevel::focus() {
  if (
    !this->xdg_toplevel->base->initialized || this->is_focused()
    || !this->mapped()
  )
    return;

  auto server = UraServer::get_instance();
  auto seat = server->seat->seat;
  auto surface = this->xdg_toplevel->base->surface;

  wlr_scene_node_raise_to_top(&this->scene_tree->node);
  wlr_xdg_toplevel_set_activated(this->xdg_toplevel, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
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
  this->lru = std::chrono::steady_clock::now().time_since_epoch().count();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-focus", args);
}

void UraToplevel::unfocus() {
  if (!this->xdg_toplevel->base->initialized)
    return;
  if (!this->is_focused())
    return;

  if (!this->destroying) {
    wlr_xdg_toplevel_set_activated(this->xdg_toplevel, false);
    wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
  }

  auto server = UraServer::get_instance();
  server->seat->text_input->unfocus_active_text_input();

  this->dismiss_popups();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-unfocus", args);
}

// get toplevel instance from wlr_surface, it asserts the surface's role is xdg_toplevel
UraToplevel* UraToplevel::from(wlr_surface* surface) {
  return static_cast<UraToplevel*>(surface->data);
}

UraToplevel* UraToplevel::from(uint64_t id) {
  auto server = UraServer::get_instance();
  if (
    server->globals.contains(id)
    && server->globals[id].type == UraGlobalType::Toplevel
  )
    return reinterpret_cast<UraToplevel*>(id);
  return nullptr;
}

void UraToplevel::activate() {
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());

  auto results =
    server->lua->emit_hook<std::vector<bool>>("pre-window-activate", args);
  if (
    results
    && std::find(results->begin(), results->end(), false) != results->end()
  )
    return;

  auto output = this->output();
  if (!output)
    return;
  output->set_tags(std::move(this->tags));
  server->seat->focus(this);
  server->lua->emit_hook("window-activate", args);
}

bool UraToplevel::move(int x, int y) {
  if (x == this->geometry.x && y == this->geometry.y)
    return false;

  auto old_output = this->prepared ? this->output() : nullptr;
  this->geometry.x = x;
  this->geometry.y = y;
  wlr_scene_node_set_position(&this->scene_tree->node, x, y);
  this->move_borders(x, y);
  if (this->prepared) {
    auto new_output = this->output();
    this->set_output(new_output);
    if (old_output != new_output) {
      if (this->is_tag_matched()) {
        this->map();
      } else {
        auto refocus = this->is_focused();
        this->unmap();
        if (refocus)
          UraServer::get_instance()->seat->focus_lru();
      }
    }
  }

  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-move", args);

  return true;
}

bool UraToplevel::resize(int width, int height) {
  if (!this->xdg_toplevel->base->initialized)
    return false;

  if (width <= 0 || height <= 0)
    return false;

  // if (this->xdg_toplevel->current.max_width > 0) {
  //   width = std::min(width, this->xdg_toplevel->current.max_width);
  // }
  if (this->xdg_toplevel->current.min_width > 0) {
    width = std::max(width, this->xdg_toplevel->current.min_width);
  }
  // if (this->xdg_toplevel->current.max_height > 0) {
  //   height = std::min(height, this->xdg_toplevel->current.max_height);
  // }
  if (this->xdg_toplevel->current.min_height > 0) {
    height = std::max(height, this->xdg_toplevel->current.min_height);
  }

  if (width == this->geometry.width && height == this->geometry.height)
    return false;

  this->geometry.width = width;
  this->geometry.height = height;
  wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);
  this->resize_borders(width, height);

  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-resize", args);
  return true;
}

void UraToplevel::close() {
  wlr_xdg_toplevel_send_close(this->xdg_toplevel);
  this->set_output(nullptr);
}

void UraToplevel::map() {
  if (
    this->mapped() || !this->xdg_toplevel->base->surface->mapped
    || !this->is_tag_matched()
  )
    return;

  auto server = UraServer::get_instance();

  auto args = flexible::create_table();
  args.set("id", this->id());
  args.set("initial", this->initial_map);

  server->lua->emit_hook("pre-window-map", args);

  wlr_scene_node_set_enabled(&this->scene_tree->node, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);
  this->set_border_visible(true);

  server->lua->emit_hook("window-map", args);

  this->initial_map = false;
}

void UraToplevel::unmap() {
  if (!this->mapped())
    return;

  auto server = UraServer::get_instance();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("pre-window-unmap", args);

  wlr_scene_node_set_enabled(&this->scene_tree->node, false);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);
  this->set_border_visible(false);

  if (server->seat->focused_toplevel() == this) {
    server->seat->unfocus();
  }

  server->lua->emit_hook("window-unmap", args);
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
  server->lua->emit_hook("window-title-change", args);
}

void UraToplevel::set_app_id(std::string app_id) {
  wlr_foreign_toplevel_handle_v1_set_app_id(
    this->foreign_handle,
    app_id.data()
  );
  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("window-app_id-change", args);
}

void UraToplevel::set_z_index(int z_index) {
  auto server = UraServer::get_instance();
  if (this->z_index != z_index) {
    auto layer = server->view->get_scene_tree_or_create(z_index);
    wlr_scene_node_reparent(&this->scene_tree->node, layer);
    this->z_index = z_index;
  }
}

bool UraToplevel::is_focused() {
  auto server = UraServer::get_instance();
  return server->seat->focused_toplevel() == this;
}

void UraToplevel::center() {
  auto output = this->output();
  if (!output)
    return;
  auto area = this->output()->usable_area;
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
  if (!flag) {
    this->pending_fullscreen_output.clear();
    this->restore_fullscreen_output();
  }
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_fullscreen(this->xdg_toplevel, flag);
  if (this->foreign_handle)
    wlr_foreign_toplevel_handle_v1_set_fullscreen(this->foreign_handle, flag);
}

void UraToplevel::set_fullscreen_output(UraOutput* output) {
  if (!output)
    return;
  if (!this->prepared) {
    this->pending_fullscreen_output = output->name;
    return;
  }
  if (this->output() == output)
    return;
  this->apply_fullscreen_output(output, true);
}

void UraToplevel::apply_fullscreen_output(
  UraOutput* output,
  bool save_restore
) {
  auto geometry = output->logical_geometry();
  if (geometry.width <= 0 || geometry.height <= 0)
    return;
  if (save_restore && !this->fullscreen_output_restore_valid) {
    this->fullscreen_output_restore_geometry = this->geometry;
    this->fullscreen_output_restore_tags = this->tags;
    this->fullscreen_output_restore_valid = true;
  }
  this->move(geometry.x, geometry.y);
  auto tags = output->tags;
  this->set_tags(std::move(tags));
}

void UraToplevel::apply_pending_fullscreen_output() {
  if (this->pending_fullscreen_output.empty())
    return;
  auto server = UraServer::get_instance();
  auto output =
    server->view->get_output_by_name(this->pending_fullscreen_output);
  this->pending_fullscreen_output.clear();
  if (output)
    this->apply_fullscreen_output(output, false);
}

void UraToplevel::restore_fullscreen_output() {
  if (!this->fullscreen_output_restore_valid)
    return;
  this->fullscreen_output_restore_valid = false;
  auto geometry = this->fullscreen_output_restore_geometry;
  auto tags = std::move(this->fullscreen_output_restore_tags);
  this->move(geometry.x, geometry.y);
  this->resize(geometry.width, geometry.height);
  this->set_tags(std::move(tags));
}

bool UraToplevel::is_fullscreen() {
  return this->xdg_toplevel->current.fullscreen;
}

void UraToplevel::set_resizing(bool flag) {
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_resizing(this->xdg_toplevel, flag);
}

bool UraToplevel::is_resizing() {
  return this->xdg_toplevel->current.resizing;
}

void UraToplevel::set_maximized(bool flag) {
  if (!this->xdg_toplevel->base->initialized)
    return;
  wlr_xdg_toplevel_set_maximized(this->xdg_toplevel, flag);
}

bool UraToplevel::is_maximized() {
  return this->xdg_toplevel->current.maximized;
}

bool UraToplevel::mapped() {
  if (!this->xdg_toplevel->base->initialized)
    return false;
  return this->xdg_toplevel->base->surface->mapped
    && this->scene_tree->node.enabled;
}

UraOutput* UraToplevel::output() {
  auto server = UraServer::get_instance();
  if (this->geometry.width <= 0 || this->geometry.height <= 0)
    return server->view->current_output();

  auto output = wlr_output_layout_output_at(
    server->output_layout,
    this->geometry.x,
    this->geometry.y
  );
  if (!output) {
    auto x = this->geometry.x + this->geometry.width / 2;
    auto y = this->geometry.y + this->geometry.height / 2;
    output = wlr_output_layout_output_at(server->output_layout, x, y);
  }
  if (output)
    return UraOutput::from(output);
  return server->view->current_output();
}

void UraToplevel::set_output(UraOutput* output) {
  if (this->foreign_output != output) {
    if (this->foreign_output) {
      wlr_foreign_toplevel_handle_v1_output_leave(
        this->foreign_handle,
        this->foreign_output->output
      );
    }
    if (output) {
      wlr_foreign_toplevel_handle_v1_output_enter(
        this->foreign_handle,
        output->output
      );
    }
    this->foreign_output = output;
  }
  this->set_scale(output ? output->scale() : 1.0);
}

void UraToplevel::update_output() {
  this->set_output(this->output());
}

void UraToplevel::output_destroyed(UraOutput* output) {
  if (this->foreign_output == output)
    this->set_output(nullptr);
}

double UraToplevel::scale() {
  auto output = this->output();
  if (output)
    return output->scale();
  return 1.;
}

void UraToplevel::set_scale(double scale) {
  if (this->preferred_scale == scale)
    return;
  this->preferred_scale = scale;
  auto server = UraServer::get_instance();
  server->view->notify_scale(this->xdg_toplevel->base->surface, scale);
}

void UraToplevel::set_tags(Vec<std::string>&& tags) {
  auto old_tags = this->tags;
  this->tags = tags;

  auto server = UraServer::get_instance();

  if (this->is_tag_matched()) {
    this->map();
    server->seat->focus(this);
  } else {
    this->unmap();
    server->seat->focus_lru();
  }

  auto args = flexible::create_table();
  args.set("id", this->id());
  args.set(
    "from",
    sol::as_table(std::vector(old_tags.begin(), old_tags.end()))
  );
  args.set(
    "to",
    sol::as_table(std::vector(this->tags.begin(), this->tags.end()))
  );
  server->lua->emit_hook("window-tags-change", args);
}

bool UraToplevel::is_tag_matched() {
  auto output = this->output();
  if (!output)
    return false;
  auto matched = false;
  for (auto& tag : output->tags) {
    if (this->tags.contains(tag)) {
      matched = true;
      break;
    }
  }
  return matched;
}

void UraToplevel::set_opacity(float opacity) {
  if (this->opacity == opacity)
    return;
  this->opacity = opacity;
  wlr_scene_node_for_each_buffer(
    &this->scene_tree->node,
    [](struct wlr_scene_buffer* buffer, int sx, int sy, void* data) {
      auto self = static_cast<UraToplevel*>(data);
      wlr_scene_buffer_set_opacity(buffer, self->opacity);
    },
    this
  );
}

void UraToplevel::create_borders() {
  auto color = util::hex2rgba(this->border_color).value();
  for (int i = 0; i < 4; i++) {
    this->borders[i] =
      wlr_scene_rect_create(this->scene_tree, 0, 0, color.data());
    wlr_scene_node_set_enabled(&this->borders[i]->node, false);
  }
}

void UraToplevel::set_border_color(std::string_view color) {
  if (auto cs = util::hex2rgba(color)) {
    this->border_color = color;
    for (auto border : this->borders) {
      wlr_scene_rect_set_color(border, cs->data());
    }
  }
}

void UraToplevel::move_borders(int x, int y) {
  auto border_width =
    this->get_userdata<unsigned int>("border_width").value_or(1);
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

void UraToplevel::resize_borders(int width, int height) {
  auto border_width =
    this->get_userdata<unsigned int>("border_width").value_or(1);
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
  // right border
  wlr_scene_node_set_position(
    &this->borders[1]->node,
    this->geometry.width,
    -border_width
  );
  // bottom border
  wlr_scene_rect_set_size(
    this->borders[2],
    width + 2 * border_width,
    border_width
  );
  // bottom border
  wlr_scene_node_set_position(
    &this->borders[2]->node,
    -border_width,
    this->geometry.height
  );
  // left border
  wlr_scene_rect_set_size(
    this->borders[3],
    border_width,
    height + 2 * border_width
  );
}

void UraToplevel::set_border_visible(bool flag) {
  for (auto border : this->borders) {
    wlr_scene_node_set_enabled(&border->node, flag);
  }
}

} // namespace ura
