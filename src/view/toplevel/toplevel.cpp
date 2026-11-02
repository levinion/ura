#include "ura/view/toplevel.hpp"
#include "ura/scene/layer.hpp"
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

UraScene& UraToplevel::scene_model() {
  return *UraServer::get_instance()->view->scene;
}

const Transform& UraToplevel::transform() {
  return this->scene_model().get<Transform>(this->entity);
}

const Geometry& UraToplevel::geometry() {
  return this->scene_model().get<Geometry>(this->entity);
}

const Visibility& UraToplevel::visibility() {
  return this->scene_model().get<Visibility>(this->entity);
}

const ZOrder& UraToplevel::z_order() {
  return this->scene_model().get<ZOrder>(this->entity);
}

const WindowTags& UraToplevel::tags() {
  return this->scene_model().get<WindowTags>(this->entity);
}

const Opacity& UraToplevel::opacity() {
  return this->scene_model().get<Opacity>(this->entity);
}

void UraToplevel::init(wlr_xdg_toplevel* xdg_toplevel) {
  auto server = UraServer::get_instance();
  this->xdg_toplevel = xdg_toplevel;
  this->entity = server->entities->create_toplevel(this);

  auto output = server->view->current_output();
  WindowTags tags;
  if (output)
    tags.values.assign(output->tags.begin(), output->tags.end());

  server->view->scene->attach_toplevel(
    this->entity,
    Transform {},
    Geometry {},
    std::move(tags),
    ZOrder { UraSceneLayer::Normal, 0 }
  );

  xdg_toplevel->base->surface->data = this;
  this->set_scale(this->scale());

  server->view->toplevels.push_back(this);

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
}

void UraToplevel::destroy() {
  auto server = UraServer::get_instance();
  this->destroying = true;
  for (auto* popup : server->seat->text_input->popups) {
    if (popup->parent == this)
      popup->parent = nullptr;
  }
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

  // Keep the entity resolvable while window-close runs, then invalidate it.
  server->entities->destroy(this->entity);
  this->entity = entt::null;
  server->view->schedule_frames();
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
      auto geometry = this->geometry();
      geometry.width = this->xdg_toplevel->base->current.geometry.width;
      geometry.height = this->xdg_toplevel->base->current.geometry.height;
      server->view->scene->set_geometry(this->entity, geometry);
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
    this->prepared = true;
    this->apply_pending_fullscreen_output();

    auto args = flexible::create_table();
    args.set("id", this->id());
    server->lua->emit_hook("window-new", args);
  }

  // update opacity
  this->set_opacity(this->opacity().value);
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

  server->view->scene->raise(this->entity);
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
  return UraServer::get_instance()->entities->toplevel(id);
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
  auto tags = this->tags().values;
  output->set_tags(Vec<std::string>(tags.begin(), tags.end()));
  server->seat->focus(this);
  server->lua->emit_hook("window-activate", args);
}

bool UraToplevel::move(int x, int y) {
  auto transform = this->transform();
  if (x == transform.x && y == transform.y)
    return false;

  auto old_output = this->prepared ? this->output() : nullptr;
  transform.x = x;
  transform.y = y;
  this->scene_model().set_transform(this->entity, transform);
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

  auto geometry = this->geometry();
  if (width == geometry.width && height == geometry.height)
    return false;

  geometry.width = width;
  geometry.height = height;
  this->scene_model().set_geometry(this->entity, geometry);
  wlr_xdg_toplevel_set_size(this->xdg_toplevel, width, height);

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

  this->scene_model().set_visible(this->entity, true);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, true);

  server->lua->emit_hook("window-map", args);

  this->initial_map = false;
}

void UraToplevel::unmap() {
  // The surface's mapped flag may already be false when the unmap signal is
  // emitted, so use our scene visibility as the source of truth here.
  if (!this->visibility().visible)
    return;

  auto server = UraServer::get_instance();

  auto args = flexible::create_table();
  args.set("id", this->id());
  server->lua->emit_hook("pre-window-unmap", args);

  this->scene_model().set_visible(this->entity, false);
  wlr_foreign_toplevel_handle_v1_set_activated(this->foreign_handle, false);

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
  if (this->z_order().layer == z_index)
    return;
  this->scene_model().set_layer(this->entity, z_index);
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
  auto transform = this->transform();
  auto geometry = this->geometry();
  auto geo = Vec4<int> {
    transform.x,
    transform.y,
    geometry.width,
    geometry.height,
  };
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
  return UraEntityRegistry::to_id(this->entity);
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
    auto transform = this->transform();
    auto geometry = this->geometry();
    this->fullscreen_output_restore_geometry = Vec4<int> {
      transform.x,
      transform.y,
      geometry.width,
      geometry.height,
    };
    auto tags = this->tags().values;
    this->fullscreen_output_restore_tags =
      Vec<std::string>(tags.begin(), tags.end());
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
    && this->visibility().visible;
}

UraOutput* UraToplevel::output() {
  auto server = UraServer::get_instance();
  auto transform = this->transform();
  auto geometry = this->geometry();
  if (geometry.width <= 0 || geometry.height <= 0)
    return server->view->current_output();

  auto output = wlr_output_layout_output_at(
    server->output_layout,
    transform.x,
    transform.y
  );
  if (!output) {
    auto x = transform.x + geometry.width / 2;
    auto y = transform.y + geometry.height / 2;
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
  auto old_tags = this->tags().values;
  WindowTags next_tags;
  next_tags.values.assign(tags.begin(), tags.end());
  this->scene_model().set_tags(this->entity, std::move(next_tags));

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
  args.set("to", sol::as_table(this->tags().values));
  server->lua->emit_hook("window-tags-change", args);
}

bool UraToplevel::is_tag_matched() {
  auto output = this->output();
  if (!output)
    return false;
  auto matched = false;
  for (auto& tag : output->tags) {
    if (this->tags().contains(tag)) {
      matched = true;
      break;
    }
  }
  return matched;
}

void UraToplevel::set_opacity(float opacity) {
  this->scene_model().set_opacity(this->entity, opacity);
}

unsigned int UraToplevel::border_width() {
  return this->get_userdata<unsigned int>("border_width").value_or(1);
}

void UraToplevel::set_border_color(std::string_view color) {
  if (!util::hex2rgba(color))
    return;
  this->border_color = color;
  UraServer::get_instance()->view->schedule_frames();
}

} // namespace ura
