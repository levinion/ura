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
  if (wlr_input_device_is_libinput(device)) {
    this->libinput_device_ = wlr_libinput_get_device_handle(device);
    this->name = libinput_device_get_name(this->libinput_device_);
  }
}

void UraPointer::set_accel_profile(std::string_view _profile) {
  auto profile = accel_profile_from_str(_profile);
  if (!profile)
    return;
  if (!this->is_libinput())
    return;
  auto device = this->libinput_device_;
  if (!libinput_device_config_accel_is_available(device)
      || libinput_device_config_accel_get_profile(device) == profile.value())
    return;
  libinput_device_config_accel_set_profile(device, profile.value());
}

bool UraPointer::is_libinput() {
  return this->libinput_device_ != nullptr;
}

} // namespace ura
