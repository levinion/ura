#include "ura/seat/tablet.hpp"
#include "ura/seat/seat.hpp"
#include "ura/core/callback.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/ura.hpp"

namespace ura {

void UraTablet::init(wlr_input_device* device) {
  auto server = UraServer::get_instance();
  this->tablet =
    wlr_tablet_create(server->tablet_manager, server->seat->seat, device);
  this->tablet->wlr_tablet->data = this->tablet;
  server->runtime->register_callback(
    &this->tablet->wlr_tablet->events.tip,
    on_tablet_tip,
    this
  );
  server->runtime->register_callback(
    &this->tablet->wlr_tablet->events.axis,
    on_tablet_axis,
    this
  );
  server->runtime->register_callback(
    &this->tablet->wlr_tablet->events.button,
    on_tablet_button,
    this
  );
  server->runtime->register_callback(
    &this->tablet->wlr_tablet->events.proximity,
    on_tablet_proximity,
    this
  );
  server->runtime
    ->register_callback(&device->events.destroy, on_tablet_destroy, this);
}

} // namespace ura
