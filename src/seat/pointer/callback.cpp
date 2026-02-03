#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/seat/pointer.hpp"
#include "ura/seat/seat.hpp"
#include "ura/ura.hpp"

namespace ura {

// TODO: impl pointer constraints protocol
void on_pointer_constraints_new_constraint(wl_listener* listener, void* data) {
  auto constraint = static_cast<wlr_pointer_constraint_v1*>(data);
  auto server = UraServer::get_instance();
  server->runtime->register_callback(
    &constraint->events.set_region,
    on_pointer_constraints_constraint_set_region,
    constraint
  );
  server->runtime->register_callback(
    &constraint->events.destroy,
    on_pointer_constraints_constraint_destroy,
    constraint
  );
}

// TODO: impl pointer_constraint
void on_pointer_constraints_constraint_set_region(
  wl_listener* listener,
  void* data
) {}

void on_pointer_constraints_constraint_destroy(
  wl_listener* listener,
  void* data
) {
  auto server = UraServer::get_instance();
  auto constraint =
    server->runtime->fetch<wlr_pointer_constraint_v1*>(listener);
  server->runtime->remove(constraint);
}

void on_new_virtual_pointer(wl_listener* listener, void* data) {
  auto virtual_pointer = static_cast<wlr_virtual_pointer_v1*>(data);
  auto device = &virtual_pointer->pointer.base;
  auto pointer = new UraPointer {};
  pointer->virt = true;
  pointer->init(device);
  auto server = UraServer::get_instance();
  server->seat->pointers.push_back(pointer);
  server->seat->cursor->attach_device(device);
}
} // namespace ura
