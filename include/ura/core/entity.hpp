#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <entt/entt.hpp>
#include "ura/util/flexible.hpp"

namespace ura {

class UraToplevel;
class UraOutput;
class UraPopup;
class UraLayerShell;
class UraSessionLockSurface;

struct ToplevelRef {
  UraToplevel* object = nullptr;
};

struct OutputRef {
  UraOutput* object = nullptr;
};

struct PopupRef {
  UraPopup* object = nullptr;
};

struct LayerShellRef {
  UraLayerShell* object = nullptr;
};

struct SessionLockRef {
  UraSessionLockSurface* object = nullptr;
};

struct LuaUserdata {
  flexible::object value;
};

class UraEntityRegistry {
public:
  entt::entity create_toplevel(UraToplevel* object);
  entt::entity create_output(UraOutput* object);
  entt::entity create_popup(UraPopup* object);
  entt::entity create_layer_shell(UraLayerShell* object);
  entt::entity create_session_lock(UraSessionLockSurface* object);
  // Detached compositor-owned scene node without a wlroots role object.
  entt::entity create_scene_node();

  UraToplevel* toplevel(entt::entity entity) const;
  UraToplevel* toplevel(uint64_t id) const;
  UraOutput* output(entt::entity entity) const;
  UraOutput* output(uint64_t id) const;
  UraPopup* popup(entt::entity entity) const;
  UraPopup* popup(uint64_t id) const;
  UraLayerShell* layer_shell(entt::entity entity) const;
  UraLayerShell* layer_shell(uint64_t id) const;
  UraSessionLockSurface* session_lock(entt::entity entity) const;
  UraSessionLockSurface* session_lock(uint64_t id) const;

  bool contains(entt::entity entity) const;
  bool contains(uint64_t id) const;
  void destroy(entt::entity entity);
  void clear();

  flexible::object userdata(uint64_t id) const;
  void set_userdata(uint64_t id, flexible::object value);
  void set_destroy_callback(std::function<void()> callback);

  template<typename T>
  std::optional<T> get_userdata(uint64_t id, std::string_view name) const {
    auto entity = from_id(id);
    if (!this->contains(entity))
      return {};
    auto* data = this->registry.try_get<LuaUserdata>(entity);
    if (!data || !data->value.is<sol::table>())
      return {};
    return data->value.as<sol::table>().get<std::optional<T>>(name);
  }

  static entt::entity from_id(uint64_t id);
  static uint64_t to_id(entt::entity entity);

  entt::registry& raw();
  const entt::registry& raw() const;

private:
  entt::registry registry;
  std::function<void()> destroy_callback;
};

} // namespace ura
