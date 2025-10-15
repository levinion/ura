#include "ura/seat/pointer.hpp"
#include <regex>
#include "flexible/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/lua/lua.hpp"
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
    this->try_apply_rules();
  }
}

void UraPointer::set_accel_profile(std::string& _profile) {
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

void UraPointer::try_apply_rules() {
  auto server = UraServer::get_instance();
  if (!this->is_libinput())
    return;
  auto rules = server->lua->fetch<sol::table>("opt.device.pointer_rules");
  if (rules) {
    for (auto const& rule : rules.value()) {
      auto [key, value] = rule;
      if (!key.is<std::string>() || !value.is<sol::table>())
        continue;
      auto pattern = key.as<std::string>();
      auto table = value.as<sol::table>();
      auto reg = std::regex(pattern);
      if (!std::regex_match(this->name, reg))
        continue;

      // TODO: sol::table to flexible::table
      // this->set_properties(table);
    }
  }
}

void UraPointer::set_properties(flexible::object& properties) {
  if (auto profile = properties.recursive_get<std::string>("accel_profile")) {
    this->set_accel_profile(profile.value());
  }
  if (auto move_speed = properties.recursive_get<double>("move_speed")) {
    this->move_speed = move_speed.value();
  }
  if (auto scroll_speed = properties.recursive_get<double>("scroll_speed")) {
    this->scroll_speed = scroll_speed.value();
  }
}

} // namespace ura
