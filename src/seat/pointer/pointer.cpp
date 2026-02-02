#include "ura/seat/pointer.hpp"
#include "ura/ura.hpp"
#include "ura/util/util.hpp"

namespace ura {

UraPointer* UraPointer::from(wlr_pointer* pointer) {
  return static_cast<UraPointer*>(pointer->data);
}

void UraPointer::init(wlr_input_device* device) {
  this->base = wlr_pointer_from_input_device(device);
  this->base->data = this;
}

std::optional<std::string> UraPointer::name() {
  auto device = this->device();
  if (!device)
    return {};
  return libinput_device_get_name(device);
}

libinput_device* UraPointer::device() {
  auto device = &this->base->base;
  if (!wlr_input_device_is_libinput(device))
    return nullptr;
  return wlr_libinput_get_device_handle(device);
}

void UraPointer::set_accel_profile(std::string_view _profile) {
  auto profile = accel_profile_from_str(_profile);
  if (!profile)
    return;
  auto device = this->device();
  if (!device || !libinput_device_config_accel_is_available(device)
      || libinput_device_config_accel_get_profile(device) == profile.value())
    return;
  // TODO: find reason why segment fault occurs when call this after resuming from another session
  libinput_device_config_accel_set_profile(device, profile.value());
}

} // namespace ura
