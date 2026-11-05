#include "ura/ura.hpp"
#include "ura/util/flexible.hpp"
#include "ura/core/server.hpp"
#include "ura/core/runtime.hpp"
#include "ura/core/callback.hpp"
#include "ura/view/client.hpp"
#include "ura/view/layer_shell.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/seat/seat.hpp"
#include <algorithm>
#include <ranges>
#include "ura/seat/text_input.hpp"
#include "ura/core/lua.hpp"
#include "ura/view/view.hpp"

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
  auto client = this->focused_client();
  if (!client || client.value().type != UraSurfaceType::Toplevel)
    return nullptr;
  return client.value().transform<UraToplevel>();
}

// only layer_shell and toplevel can be focused
std::optional<UraClient> UraSeat::focused_client() {
  // Keep a logical focus independently from wlr_seat's keyboard surface. A
  // seat can temporarily have no keyboard (headless, device removal, session
  // lock) while a toplevel is still the active logical window.
  if (this->logical_focused)
    return UraClient::from(this->logical_focused);
  if (!this->seat->keyboard_state.focused_surface)
    return {};
  auto client = UraClient::from(this->seat->keyboard_state.focused_surface);
  return client;
}

// this function only handles keyboard focus. pointer focus is handled by cursor
void UraSeat::unfocus() {
  auto server = UraServer::get_instance();
  auto client = this->focused_client();
  if (client) {
    if (client->type == UraSurfaceType::Toplevel)
      client->transform<UraToplevel>()->unfocus();
    else if (client->type == UraSurfaceType::LayerShell)
      client->transform<UraLayerShell>()->unfocus();
    else if (client->type == UraSurfaceType::Popup)
      client->transform<UraPopup>()->unfocus();
    server->seat->cursor->set_xcursor("left_ptr");
  }
  this->logical_focused = nullptr;
  if (this->seat->keyboard_state.focused_surface)
    wlr_seat_keyboard_notify_clear_focus(seat);
}

void UraSeat::focus(UraClient client) {
  if (
    !client.surface || client.type == UraSurfaceType::Popup
    || client.type == UraSurfaceType::SessionLock
    || client.type == UraSurfaceType::Null
    || client.type == UraSurfaceType::Unknown
  )
    return;

  auto focused = this->focused_client();
  if (focused && *focused == client)
    return;

  if (client.type == UraSurfaceType::Toplevel) {
    auto* toplevel = client.transform<UraToplevel>();
    if (!toplevel->xdg_toplevel->base->initialized || !toplevel->mapped())
      return;
  }

  if (focused)
    this->unfocus();

  client.focus();
  if (client.type == UraSurfaceType::Toplevel)
    this->logical_focused = client.transform<UraToplevel>();
  else
    this->logical_focused = nullptr;
}

void UraSeat::focus(UraToplevel* toplevel) {
  this->focus(UraClient::from(toplevel));
}

void UraSeat::focus(UraLayerShell* layer_shell) {
  this->focus(UraClient::from(layer_shell));
}

void UraSeat::focus_lru() {
  auto server = UraServer::get_instance();
  auto toplevels = server->view->toplevels
    | std::views::filter([](auto v) { return v->mapped(); })
    | std::ranges::to<std::vector<UraToplevel*>>();
  auto toplevel =
    std::max_element(toplevels.begin(), toplevels.end(), [](auto a, auto b) {
      return a->lru < b->lru;
    });
  // auto layer_shells = server->view->layer_shells()
  //   | std::views::filter([](auto v) { return v->focusable(); })
  //   | std::ranges::to<std::vector<UraLayerShell*>>();
  // auto layer_shell = std::max_element(
  //   layer_shells.begin(),
  //   layer_shells.end(),
  //   [](auto a, auto b) { return a->lru < b->lru; }
  // );
  // if (toplevel != toplevels.end() && layer_shell != layer_shells.end()) {
  //   if ((*toplevel)->lru > (*layer_shell)->lru)
  //     server->seat->focus(*toplevel);
  //   else {
  //     server->seat->focus(*layer_shell);
  //   }
  // } else if (toplevel != toplevels.end()) {
  //   server->seat->focus(*toplevel);
  // } else if (layer_shell != layer_shells.end()) {
  //   server->seat->focus(*layer_shell);
  // }
  if (toplevel != toplevels.end())
    server->seat->focus(*toplevel);
  else
    this->logical_focused = nullptr;
}

void UraSeat::notify_idle_activity() {
  auto server = UraServer::get_instance();
  server->view->wake_outputs();
  this->cursor->notify_activity();
  wlr_idle_notifier_v1_notify_activity(server->idle_notifier, this->seat);

  if (this->idle) {
    this->idle = false;
    auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - this->idle_since
    );
    auto args = flexible::create_table();
    args.set("idle", elapsed.count());

    std::vector<sol::protected_function> callbacks;
    for (auto& stage : this->idle_timeouts) {
      if (!stage.fired)
        continue;
      stage.fired = false;
      if (stage.on_resume)
        callbacks.push_back(*stage.on_resume);
    }
    for (auto& callback : callbacks) {
      auto result = callback(args);
      if (!result.valid()) {
        sol::error err = result;
        log::error("[lua] idle resume callback: {}", err.what());
      }
    }
    server->lua->emit_hook("resume", args);
  }
  this->restart_idle_timers();
}

void UraSeat::set_idle_inhibitor(bool flag) {
  auto server = UraServer::get_instance();
  this->idle_inhibited = flag;
  wlr_idle_notifier_v1_set_inhibited(server->idle_notifier, flag);
  if (flag)
    this->cancel_idle_timers();
  else
    this->restart_idle_timers();
}

void UraSeat::clear_idle_timeouts() {
  this->cancel_idle_timers();
  this->idle_timeouts.clear();
  this->idle = false;
}

void UraSeat::add_idle_timeout(
  std::string name,
  std::chrono::milliseconds timeout,
  std::optional<sol::protected_function> on_idle,
  std::optional<sol::protected_function> on_resume
) {
  if (name.empty())
    name = "default";
  this->cancel_idle_timers();
  this->idle = false;

  auto it = std::ranges::find_if(this->idle_timeouts, [&name](auto& value) {
    return value.name == name;
  });
  if (timeout.count() <= 0) {
    if (it != this->idle_timeouts.end())
      this->idle_timeouts.erase(it);
    this->restart_idle_timers();
    return;
  }

  if (it != this->idle_timeouts.end()) {
    it->timeout = timeout;
    it->fired = false;
    it->timer = -1;
    it->on_idle = std::move(on_idle);
    it->on_resume = std::move(on_resume);
  } else {
    this->idle_timeouts.push_back(
      IdleTimeout {
        .name = std::move(name),
        .timeout = timeout,
        .on_idle = std::move(on_idle),
        .on_resume = std::move(on_resume),
      }
    );
  }
  this->restart_idle_timers();
}

void UraSeat::remove_idle_timeout(const std::string& name) {
  this->cancel_idle_timers();
  this->idle = false;
  std::erase_if(this->idle_timeouts, [&name](auto& value) {
    return value.name == name;
  });
  this->restart_idle_timers();
}

bool UraSeat::is_idle() const {
  return this->idle;
}

void UraSeat::cancel_idle_timers() {
  auto server = UraServer::get_instance();
  for (auto& stage : this->idle_timeouts) {
    if (stage.timer != -1) {
      server->dispatcher->clear_timer(stage.timer);
      stage.timer = -1;
    }
    stage.fired = false;
  }
}

void UraSeat::restart_idle_timers() {
  this->cancel_idle_timers();
  if (this->idle_inhibited || this->idle)
    return;

  auto server = UraServer::get_instance();
  for (auto& stage : this->idle_timeouts) {
    if (stage.timeout.count() <= 0)
      continue;
    auto name = stage.name;
    stage.timer = server->dispatcher->set_timeout(
      [this, name]() { this->on_idle_stage(name); },
      stage.timeout
    );
  }
}

void UraSeat::on_idle_stage(const std::string& name) {
  auto it = std::ranges::find_if(this->idle_timeouts, [&name](auto& value) {
    return value.name == name;
  });
  if (it == this->idle_timeouts.end() || it->fired)
    return;

  it->fired = true;
  it->timer = -1;
  if (!this->idle) {
    this->idle = true;
    this->idle_since = std::chrono::steady_clock::now();
  }

  auto server = UraServer::get_instance();
  auto args = flexible::create_table();
  args.set("name", it->name);
  args.set("timeout", std::chrono::duration<double>(it->timeout).count());
  if (it->on_idle) {
    auto result = (*it->on_idle)(args);
    if (!result.valid()) {
      sol::error err = result;
      log::error("[lua] idle callback: {}", err.what());
    }
  }
  server->lua->emit_hook("idle", args);
}

void UraSeat::update_capability() {
  uint32_t caps = 0;
  if (!this->pointers.empty() || !this->tablets.empty()) {
    caps |= WL_SEAT_CAPABILITY_POINTER;
  }
  if (!this->keyboards.empty()) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(this->seat, caps);
}

void UraSeat::attach_new_input(wlr_input_device* device) {
  // TODO: enable libinput lua plugins
  // This should be called before iterating devices,
  // which means it should be implemented by wlroots' libinput backend.
  // We could do nothing without wlroots impl that.

  // static auto libinput_plugin_loaded = false;
  // if (!libinput_plugin_loaded && wlr_input_device_is_libinput(device)) {
  //   auto handle = wlr_libinput_get_device_handle(device);
  //   auto context = libinput_device_get_context(handle);
  //   libinput_plugin_system_append_default_paths(context);
  //   if (libinput_plugin_system_load_plugins(
  //         context,
  //         LIBINPUT_PLUGIN_SYSTEM_FLAG_NONE
  //       )
  //       == 0) {
  //     libinput_plugin_loaded = true;
  //     log::info("LIBINPUT LUA PLUGINS LOADED");
  //   } else {
  //     log::error("FAILED TO LOAD LIBINPUT LUA PLUGINS");
  //   }
  // }

  auto server = UraServer::get_instance();

  switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
      auto keyboard = new UraKeyboard {};
      keyboard->init(device);
      this->keyboards.push_back(keyboard);
      break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
      this->cursor->attach_device(device);
      auto pointer = new UraPointer {};
      pointer->init(device);
      this->pointers.push_back(pointer);
      break;
    }
    case WLR_INPUT_DEVICE_TABLET: {
      this->cursor->attach_device(device);
      auto tablet = new UraTablet {};
      tablet->init(device);
      this->tablets.push_back(tablet);
      break;
    }
    case WLR_INPUT_DEVICE_TOUCH: {
      server->seat->cursor->attach_device(device);
      break;
    }
    default:
      break;
  }

  server->seat->update_capability();

  auto args = flexible::create_table();
  args.set("name", device->name);
  server->lua->emit_hook("new-input", args);
}

uint32_t UraSeat::get_modifiers() {
  uint32_t modifiers = 0u;
  for (auto keyboard : this->keyboards) {
    modifiers |= wlr_keyboard_get_modifiers(keyboard->keyboard);
  }
  return modifiers;
}

} // namespace ura
