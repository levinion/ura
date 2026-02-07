#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/session_lock.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/seat/seat.hpp"

namespace ura {
void on_new_session_lock(wl_listener* listener, void* data) {
  auto session_lock = static_cast<wlr_session_lock_v1*>(data);
  auto server = UraServer::get_instance();
  auto lock = new UraSessionLock;
  lock->lock = session_lock;
  lock->scene_tree = wlr_scene_tree_create(
    server->view->get_scene_tree_or_create(UraSceneLayer::LockScreen)
  );
  server->runtime->register_callback(
    &session_lock->events.new_surface,
    on_session_lock_new_surface,
    lock
  );
  server->runtime->register_callback(
    &session_lock->events.unlock,
    on_session_lock_unlock,
    lock
  );
  server->runtime->register_callback(
    &session_lock->events.destroy,
    on_session_lock_destroy,
    lock
  );
  wlr_session_lock_v1_send_locked(session_lock);
}

void on_session_lock_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock = server->runtime->fetch<UraSessionLock*>(listener);
  server->runtime->remove(lock);
  delete lock;
}

void on_session_lock_unlock(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock = server->runtime->fetch<UraSessionLock*>(listener);
  server->seat->locked = false;
  server->seat->unfocus();
  wlr_scene_node_set_enabled(&lock->scene_tree->node, false);
  server->seat->cursor->show();
  auto output = server->view->current_output();
  if (output)
    output->focus_lru();
}

void on_session_lock_new_surface(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock = server->runtime->fetch<UraSessionLock*>(listener);
  auto surface = static_cast<wlr_session_lock_surface_v1*>(data);

  auto lock_surface = new UraSessionLockSurface;
  lock_surface->surface = surface;
  auto scene_tree =
    wlr_scene_subsurface_tree_create(lock->scene_tree, surface->surface);
  lock_surface->scene_tree = scene_tree;
  auto output = UraOutput::from(surface->output);
  lock_surface->output = output;
  output->session_lock_surface = lock_surface;

  surface->surface->data = lock_surface;

  auto geo = output->logical_geometry();
  wlr_scene_node_set_position(&scene_tree->node, geo.x, geo.y);
  wlr_session_lock_surface_v1_configure(surface, geo.width, geo.height);
  server->runtime->register_callback(
    &surface->events.destroy,
    on_session_lock_surface_destroy,
    lock_surface
  );

  server->seat->unfocus();
  server->seat->cursor->hide();
  server->seat->locked = true;
  lock_surface->focus();
}

void on_session_lock_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock_surface = server->runtime->fetch<UraSessionLockSurface*>(listener);
  lock_surface->output->session_lock_surface = nullptr;
  server->runtime->remove(lock_surface);
  delete lock_surface;
}
} // namespace ura
