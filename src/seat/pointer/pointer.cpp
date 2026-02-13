#include "ura/seat/pointer.hpp"
#include "ura/ura.hpp"

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

} // namespace ura
