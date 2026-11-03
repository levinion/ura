#include "ura/render/effect_registry.hpp"

#include <utility>

namespace ura {

EffectHandle EffectRegistry::define(EffectPipeline pipeline) {
  auto name = pipeline.name;
  auto existing = this->names.find(name);
  if (existing != this->names.end()) {
    this->pipelines.erase(existing->second.id);
    this->names.erase(existing);
  }

  auto handle = EffectHandle {
    .id = this->next_id++,
    .generation = this->generation,
  };
  pipeline.handle = handle;
  this->pipelines[handle.id] =
    std::make_shared<const EffectPipeline>(std::move(pipeline));
  this->names[name] = handle;
  return handle;
}

std::shared_ptr<const EffectPipeline>
EffectRegistry::get(const EffectHandle& handle) const {
  auto it = this->pipelines.find(handle.id);
  if (it == this->pipelines.end())
    return {};
  if (it->second->handle.generation != handle.generation)
    return {};
  return it->second;
}

bool EffectRegistry::remove(const EffectHandle& handle) {
  auto it = this->pipelines.find(handle.id);
  if (it == this->pipelines.end())
    return false;
  if (it->second->handle.generation != handle.generation)
    return false;
  auto name = it->second->name;
  this->pipelines.erase(it);
  auto name_it = this->names.find(name);
  if (name_it != this->names.end() && name_it->second.id == handle.id)
    this->names.erase(name_it);
  return true;
}

void EffectRegistry::clear() {
  this->pipelines.clear();
  this->names.clear();
  this->textures.clear();
  this->texture_names.clear();
}

EffectTextureHandle EffectRegistry::define_texture(EffectTexture texture) {
  if (!texture.name.empty()) {
    auto existing = this->texture_names.find(texture.name);
    if (existing != this->texture_names.end()) {
      this->textures.erase(existing->second.id);
      this->texture_names.erase(existing);
    }
  }

  auto handle = EffectTextureHandle {
    .id = this->next_texture_id++,
    .generation = this->texture_generation,
  };
  texture.handle = handle;
  if (!texture.name.empty())
    this->texture_names[texture.name] = handle;
  this->textures[handle.id] =
    std::make_shared<const EffectTexture>(std::move(texture));
  return handle;
}

std::shared_ptr<const EffectTexture>
EffectRegistry::find_texture(std::string_view name) const {
  auto it = this->texture_names.find(std::string(name));
  if (it == this->texture_names.end())
    return {};
  return this->get_texture(it->second);
}

std::shared_ptr<const EffectTexture>
EffectRegistry::get_texture(const EffectTextureHandle& handle) const {
  auto it = this->textures.find(handle.id);
  if (it == this->textures.end())
    return {};
  if (it->second->handle.generation != handle.generation)
    return {};
  return it->second;
}

bool EffectRegistry::remove_texture(const EffectTextureHandle& handle) {
  auto it = this->textures.find(handle.id);
  if (it == this->textures.end())
    return false;
  if (it->second->handle.generation != handle.generation)
    return false;
  auto name = it->second->name;
  this->textures.erase(it);
  auto name_it = this->texture_names.find(name);
  if (name_it != this->texture_names.end() && name_it->second.id == handle.id)
    this->texture_names.erase(name_it);
  return true;
}

std::shared_ptr<const EffectPipeline>
EffectRegistry::find(std::string_view name) const {
  auto it = this->names.find(std::string(name));
  if (it == this->names.end())
    return {};
  return this->get(it->second);
}

uint64_t EffectRegistry::next_instance_version() {
  return this->next_version++;
}

} // namespace ura
