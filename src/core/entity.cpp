#include "ura/core/entity.hpp"
#include <limits>

namespace ura {

namespace {

template<typename Ref>
entt::entity create_ref(entt::registry& registry, Ref ref) {
  auto entity = registry.create();
  registry.emplace<Ref>(entity, ref);
  registry.emplace<LuaUserdata>(entity);
  return entity;
}

template<typename Ref, typename T>
T* lookup(const entt::registry& registry, entt::entity entity) {
  if (entity == entt::null || !registry.valid(entity))
    return nullptr;
  auto* ref = registry.try_get<Ref>(entity);
  return ref ? ref->object : nullptr;
}

} // namespace

entt::entity UraEntityRegistry::create_toplevel(UraToplevel* object) {
  return create_ref(this->registry, ToplevelRef { object });
}

entt::entity UraEntityRegistry::create_output(UraOutput* object) {
  return create_ref(this->registry, OutputRef { object });
}

entt::entity UraEntityRegistry::create_popup(UraPopup* object) {
  return create_ref(this->registry, PopupRef { object });
}

entt::entity UraEntityRegistry::create_layer_shell(UraLayerShell* object) {
  return create_ref(this->registry, LayerShellRef { object });
}

UraToplevel* UraEntityRegistry::toplevel(entt::entity entity) const {
  return lookup<ToplevelRef, UraToplevel>(this->registry, entity);
}

UraToplevel* UraEntityRegistry::toplevel(uint64_t id) const {
  return this->toplevel(from_id(id));
}

UraOutput* UraEntityRegistry::output(entt::entity entity) const {
  return lookup<OutputRef, UraOutput>(this->registry, entity);
}

UraOutput* UraEntityRegistry::output(uint64_t id) const {
  return this->output(from_id(id));
}

UraPopup* UraEntityRegistry::popup(entt::entity entity) const {
  return lookup<PopupRef, UraPopup>(this->registry, entity);
}

UraPopup* UraEntityRegistry::popup(uint64_t id) const {
  return this->popup(from_id(id));
}

UraLayerShell* UraEntityRegistry::layer_shell(entt::entity entity) const {
  return lookup<LayerShellRef, UraLayerShell>(this->registry, entity);
}

UraLayerShell* UraEntityRegistry::layer_shell(uint64_t id) const {
  return this->layer_shell(from_id(id));
}

bool UraEntityRegistry::contains(entt::entity entity) const {
  return entity != entt::null && this->registry.valid(entity);
}

bool UraEntityRegistry::contains(uint64_t id) const {
  return this->contains(from_id(id));
}

void UraEntityRegistry::destroy(entt::entity entity) {
  if (!this->contains(entity))
    return;
  this->registry.destroy(entity);
}

void UraEntityRegistry::clear() {
  this->registry.clear();
}

flexible::object UraEntityRegistry::userdata(uint64_t id) const {
  auto entity = from_id(id);
  if (!this->contains(entity))
    return {};
  auto* data = this->registry.try_get<LuaUserdata>(entity);
  return data ? data->value : flexible::object {};
}

void UraEntityRegistry::set_userdata(uint64_t id, flexible::object value) {
  auto entity = from_id(id);
  if (!this->contains(entity))
    return;
  if (auto* data = this->registry.try_get<LuaUserdata>(entity)) {
    data->value = value;
    return;
  }
  this->registry.emplace<LuaUserdata>(entity, value);
}

entt::entity UraEntityRegistry::from_id(uint64_t id) {
  if (id == 0)
    return entt::null;
  auto raw = id - 1;
  if (raw > std::numeric_limits<entt::id_type>::max())
    return entt::null;
  return static_cast<entt::entity>(raw);
}

uint64_t UraEntityRegistry::to_id(entt::entity entity) {
  if (entity == entt::null)
    return 0;
  return static_cast<uint64_t>(entt::to_integral(entity)) + 1;
}

entt::registry& UraEntityRegistry::raw() {
  return this->registry;
}

const entt::registry& UraEntityRegistry::raw() const {
  return this->registry;
}

} // namespace ura
