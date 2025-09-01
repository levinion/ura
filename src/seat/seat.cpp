#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/view/client.hpp"
#include "ura/view/view.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/seat.hpp"
#include "ura/seat/text_input.hpp"
#include "ura/lua/lua.hpp"
#include <regex>

namespace ura {
void UraSeat::init() {
  auto server = UraServer::get_instance();
  this->seat = wlr_seat_create(server->display, "seat0");
  server->runtime->register_callback(
    &server->backend->events.new_input,
    on_new_input,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_cursor,
    on_seat_request_cursor,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_selection,
    on_seat_request_set_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_set_primary_selection,
    on_seat_request_set_primary_selection,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.request_start_drag,
    on_seat_request_start_drag,
    nullptr
  );
  server->runtime->register_callback(
    &this->seat->events.start_drag,
    on_seat_start_drag,
    nullptr
  );
  this->cursor = std::make_unique<UraCursor>();
  this->cursor->init();
  this->text_input = std::make_unique<UraTextInput>();
}

UraToplevel* UraSeat::focused_toplevel() {
  if (!this->seat->keyboard_state.focused_surface)
    return nullptr;
  auto client = UraClient::from(this->seat->keyboard_state.focused_surface);
  if (client.type == UraSurfaceType::Toplevel)
    return client.transform<UraToplevel>();
  return nullptr;
}

std::optional<UraClient> UraSeat::focused_client() {
  if (!this->seat->keyboard_state.focused_surface)
    return {};
  auto client = UraClient::from(this->seat->keyboard_state.focused_surface);
  return client;
}

void UraSeat::unfocus() {
  auto server = UraServer::get_instance();
  if (!this->seat->keyboard_state.focused_surface
      && !this->seat->pointer_state.focused_surface)
    return;
  if (this->seat->keyboard_state.focused_surface) {
    auto toplevel = this->focused_toplevel();
    if (toplevel)
      toplevel->unfocus();
    wlr_seat_keyboard_notify_clear_focus(seat);
  }
  if (this->seat->pointer_state.focused_surface) {
    wlr_seat_pointer_notify_clear_focus(seat);
  }
  this->cursor->set_xcursor("left_ptr");
  server->lua->try_execute_hook("focus-change");
}

void UraSeat::focus(UraClient client) {
  if (!client.surface || client.type == UraSurfaceType::Popup
      || client.type == UraSurfaceType::SessionLock
      || client.type == UraSurfaceType::Unknown
      || this->seat->keyboard_state.focused_surface == client.surface)
    return;
  auto focused = this->focused_client();
  if (focused)
    this->unfocus();
  client.focus();
  auto server = UraServer::get_instance();
  server->lua->try_execute_hook("focus-change");
}

void UraSeat::focus(UraToplevel* toplevel) {
  this->focus(UraClient::from(toplevel));
}

void UraSeat::focus(UraLayerShell* layer_shell) {
  this->focus(UraClient::from(layer_shell));
}

void UraSeat::notify_idle_activity() {
  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_notify_activity(server->idle_notifier, this->seat);
  if (!server->idle_notifier->WLR_PRIVATE.inhibited)
    for (auto [_, output] : server->view->outputs) {
      if (!output->dpms_on)
        output->set_dpms_mode(true);
    }
}

void UraSeat::set_idle_inhibitor(bool flag) {
  auto server = UraServer::get_instance();
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, flag);
}

std::optional<libinput_config_accel_profile>
accel_profile_from_str(std::string_view profile) {
  if (profile == "flat")
    return LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
  else if (profile == "adaptive")
    return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
  return {};
}

void UraSeat::set_pointer_accel_profile(
  wlr_input_device* wlr_device,
  std::string& _profile
) {
  auto profile = accel_profile_from_str(_profile);
  if (!profile)
    return;
  if (!wlr_input_device_is_libinput(wlr_device))
    return;
  auto device = wlr_libinput_get_device_handle(wlr_device);
  if (!libinput_device_config_accel_is_available(device)
      || libinput_device_config_accel_get_profile(device) == profile.value())
    return;
  libinput_device_config_accel_set_profile(device, profile.value());
}

void UraSeat::set_pointer_accel_profile(
  std::string& pattern,
  std::string& _profile
) {
  auto reg = std::regex(pattern);
  for (auto wlr_device : this->devices) {
    if (!wlr_input_device_is_libinput(wlr_device))
      continue;
    auto device = wlr_libinput_get_device_handle(wlr_device);
    std::string device_name = libinput_device_get_name(device);
    if (std::regex_match(device_name, reg)) {
      this->set_pointer_accel_profile(wlr_device, _profile);
    }
  }
}

void UraSeat::try_apply_pointer_rules(wlr_input_device* wlr_device) {
  auto server = UraServer::get_instance();
  if (!wlr_input_device_is_libinput(wlr_device))
    return;
  auto device = wlr_libinput_get_device_handle(wlr_device);
  std::string device_name = libinput_device_get_name(device);
  auto rules = server->lua->fetch<sol::table>("opt.device.pointer_rules");
  if (rules) {
    for (auto const& rule : rules.value()) {
      auto [key, value] = rule;
      if (!key.is<std::string>() || !value.is<sol::table>())
        continue;
      auto pattern = key.as<std::string>();
      auto table = value.as<sol::table>();
      auto reg = std::regex(pattern);
      if (!std::regex_match(device_name, reg))
        continue;
      auto profile = table.get<std::optional<std::string>>("accel_profile");
      if (profile)
        this->set_pointer_accel_profile(wlr_device, profile.value());
      // TODO: more rules should be added here
    }
  }
}

} // namespace ura
