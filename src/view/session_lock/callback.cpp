#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/view/session_lock.hpp"
#include "ura/view/output.hpp"
#include "ura/view/view.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/lua.hpp"

namespace ura {
namespace {
  void restore_session_after_unlock(UraSessionLock* lock) {
    auto server = UraServer::get_instance();
    auto was_locked = server->seat->locked;
    server->seat->locked = false;
    server->seat->unfocus();
    server->seat->cursor->set_visible(true);
    server->seat->focus_lru();
    // Do not leave a stale lock surface over the desktop if the lock client
    // has not destroyed it yet.
    for (auto& [_, output] : server->view->outputs) {
      auto* lock_surface = output ? output->session_lock_surface : nullptr;
      if (lock_surface && lock_surface->entity != entt::null)
        server->view->scene->set_visible(lock_surface->entity, false);
    }
    server->view->schedule_frames();
    if (was_locked && server->lua)
      server->lua->emit_hook("session-unlock", {});
  }
} // namespace

void on_new_session_lock(wl_listener* listener, void* data) {
  auto session_lock = static_cast<wlr_session_lock_v1*>(data);
  auto server = UraServer::get_instance();
  auto lock = new UraSessionLock;
  lock->lock = session_lock;
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
  // A killed lock client may be destroyed without sending the unlock event.
  if (server->seat->locked)
    restore_session_after_unlock(lock);
  server->runtime->remove(lock);
  delete lock;
}

void on_session_lock_unlock(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock = server->runtime->fetch<UraSessionLock*>(listener);
  restore_session_after_unlock(lock);
}

void on_session_lock_new_surface(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto surface = static_cast<wlr_session_lock_surface_v1*>(data);

  auto output = UraOutput::from(surface->output);
  if (!output)
    return;

  auto lock_surface = new UraSessionLockSurface;
  lock_surface->surface = surface;
  lock_surface->output = output;
  lock_surface->entity = server->entities->create_session_lock(lock_surface);
  output->session_lock_surface = lock_surface;

  surface->surface->data = lock_surface;

  auto geo = output->logical_geometry();
  Rect<double> bounds {
    .x = static_cast<double>(geo.x),
    .y = static_cast<double>(geo.y),
    .width = static_cast<double>(geo.width),
    .height = static_cast<double>(geo.height),
  };
  server->view->scene->attach_session_lock(lock_surface->entity, bounds);
  wlr_session_lock_surface_v1_configure(surface, geo.width, geo.height);
  server->runtime->register_callback(
    &surface->events.destroy,
    on_session_lock_surface_destroy,
    lock_surface
  );
  server->runtime->register_callback(
    &surface->surface->events.commit,
    on_session_lock_surface_commit,
    lock_surface
  );
  server->runtime->register_callback(
    &surface->surface->events.map,
    on_session_lock_surface_commit,
    lock_surface
  );
  server->runtime->register_callback(
    &surface->surface->events.unmap,
    on_session_lock_surface_commit,
    lock_surface
  );

  auto was_locked = server->seat->locked;
  server->seat->unfocus();
  server->seat->cursor->set_visible(false);
  server->seat->locked = true;
  lock_surface->focus();
  server->view->schedule_frames();
  if (!was_locked && server->lua)
    server->lua->emit_hook("session-lock", {});
}

void on_session_lock_surface_commit(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock_surface = server->runtime->fetch<UraSessionLockSurface*>(listener);
  auto* output = lock_surface->output;

  if (lock_surface->entity != entt::null) {
    auto* scene = server->view->scene.get();
    lock_surface->update_scene_geometry();
    auto* state = &lock_surface->surface->surface->current;
    if (state->width > 0 && state->height > 0) {
      scene->set_surface_content(
        lock_surface->entity,
        SurfaceContent {
          .geometry =
            Rect<int> {
              .width = state->width,
              .height = state->height,
            },
        }
      );
    }
    lock_surface->update_visibility();
  }

  if (!output || !output->output || !output->output->enabled) {
    server->view->schedule_frames();
    return;
  }

  // Lock surfaces live outside the regular Scene commit path, so their
  // per-commit damage still needs an explicit schedule.
  server->view->frame_scheduler->mark_output_dirty(output);
  wlr_output_schedule_frame(output->output);
}

void on_session_lock_surface_destroy(wl_listener* listener, void* data) {
  auto server = UraServer::get_instance();
  auto lock_surface = server->runtime->fetch<UraSessionLockSurface*>(listener);
  if (lock_surface->output)
    lock_surface->output->session_lock_surface = nullptr;
  server->runtime->remove(lock_surface);
  if (lock_surface->entity != entt::null) {
    server->view->scene->set_visible(lock_surface->entity, false);
    server->entities->destroy(lock_surface->entity);
    lock_surface->entity = entt::null;
  }
  delete lock_surface;
  server->view->schedule_frames();
}
} // namespace ura
