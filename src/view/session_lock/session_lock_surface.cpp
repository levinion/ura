#include "ura/view/session_lock.hpp"
#include "ura/core/server.hpp"
#include "ura/seat/seat.hpp"
#include "ura/view/view.hpp"

namespace ura {

void UraSessionLockSurface::focus() {
  auto server = UraServer::get_instance();
  auto keyboard = wlr_seat_get_keyboard(server->seat->seat);
  if (keyboard) {
    wlr_seat_keyboard_notify_enter(
      server->seat->seat,
      this->surface->surface,
      keyboard->keycodes,
      keyboard->num_keycodes,
      &keyboard->modifiers
    );
  }
}

UraSessionLockSurface* UraSessionLockSurface::from(wlr_surface* surface) {
  return static_cast<UraSessionLockSurface*>(surface->data);
}

void UraSessionLockSurface::update_scene_geometry() {
  if (this->entity == entt::null || !this->output)
    return;

  auto* server = UraServer::get_instance();
  auto geometry = this->output->logical_geometry();
  Rect<double> bounds {
    .x = static_cast<double>(geometry.x),
    .y = static_cast<double>(geometry.y),
    .width = static_cast<double>(geometry.width),
    .height = static_cast<double>(geometry.height),
  };
  server->view->scene->set_layout_target(this->entity, bounds);
  server->view->scene->set_node_bounds(this->entity, bounds);
}

void UraSessionLockSurface::update_visibility() {
  if (this->entity == entt::null)
    return;

  auto* server = UraServer::get_instance();
  auto enabled =
    this->output && this->output->output && this->output->output->enabled;
  server->view->scene->set_visible(
    this->entity,
    enabled && this->surface->surface->mapped
  );
}

} // namespace ura
