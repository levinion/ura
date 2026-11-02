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

// Wrappers keep the wlroots-facing object alive while the registry owns its
// identity. The component is also the type tag used by the ID lookups below.
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

struct LuaUserdata {
  flexible::object value;
};

// Process-local identity for objects exposed to Lua.
//
// The public handle is derived from entt::entity, so destroying an entity
// invalidates its old handle even when EnTT recycles the underlying index.
// EnTT entities are 32-bit; adding one keeps 0 as an invalid handle and keeps
// every valid handle exactly representable by LuaJIT numbers.
class UraEntityRegistry {
public:
  entt::entity create_toplevel(UraToplevel* object);
  entt::entity create_output(UraOutput* object);
  entt::entity create_popup(UraPopup* object);
  entt::entity create_layer_shell(UraLayerShell* object);

  UraToplevel* toplevel(entt::entity entity) const;
  UraToplevel* toplevel(uint64_t id) const;
  UraOutput* output(entt::entity entity) const;
  UraOutput* output(uint64_t id) const;
  UraPopup* popup(entt::entity entity) const;
  UraPopup* popup(uint64_t id) const;
  UraLayerShell* layer_shell(entt::entity entity) const;
  UraLayerShell* layer_shell(uint64_t id) const;

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
