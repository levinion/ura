#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "ura/render/effect.hpp"

namespace ura {

class EffectRegistry {
public:
  EffectHandle define(EffectPipeline pipeline);
  std::shared_ptr<const EffectPipeline> get(const EffectHandle& handle) const;
  EffectTextureHandle define_texture(EffectTexture texture);
  std::shared_ptr<const EffectTexture>
  get_texture(const EffectTextureHandle& handle) const;
  std::shared_ptr<const EffectTexture>
  find_texture(std::string_view name) const;
  bool remove_texture(const EffectTextureHandle& handle);
  std::shared_ptr<const EffectPipeline> find(std::string_view name) const;
  bool remove(const EffectHandle& handle);
  void clear();

  uint64_t next_instance_version();

private:
  uint64_t next_id = 1;
  uint32_t generation = 1;
  uint64_t next_version = 1;
  std::unordered_map<uint64_t, std::shared_ptr<const EffectPipeline>> pipelines;
  std::unordered_map<std::string, EffectHandle> names;
  uint64_t next_texture_id = 1;
  uint32_t texture_generation = 1;
  std::unordered_map<uint64_t, std::shared_ptr<const EffectTexture>> textures;
  std::unordered_map<std::string, EffectTextureHandle> texture_names;
};

} // namespace ura
