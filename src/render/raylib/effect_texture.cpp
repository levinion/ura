#include "ura/render/raylib/renderer.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

namespace ura::render::raylib {

struct EffectTextureProgram {
  std::weak_ptr<const EffectTexture> texture;
  GLuint id = 0;
  int width = 0;
  int height = 0;
};

struct EffectTextureCache {
  std::unordered_map<const EffectTexture*, EffectTextureProgram> textures;
};

static void effect_texture_destroy(EffectTextureProgram& program) {
  if (program.id)
    glDeleteTextures(1, &program.id);
  program.id = 0;
  program.width = 0;
  program.height = 0;
}

GLuint effect_texture_get(Renderer* renderer, const EffectTexture& texture) {
  if (!renderer || texture.width <= 0 || texture.height <= 0)
    return 0;
  if (
    texture.pixels.size()
    < static_cast<std::size_t>(texture.width * texture.height * 4)
  )
    return 0;
  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  if (
    max_texture_size > 0
    && (texture.width > max_texture_size || texture.height > max_texture_size)
  )
    return 0;

  if (!renderer->effect_textures)
    renderer->effect_textures = new EffectTextureCache;

  auto* key = &texture; // Lifetime is anchored by the pipeline shared_ptr.
  auto& cache = *renderer->effect_textures;
  auto it = cache.textures.find(key);
  if (it != cache.textures.end()) {
    auto existing = it->second.texture.lock();
    if (existing.get() == key)
      return it->second.id;
    effect_texture_destroy(it->second);
    cache.textures.erase(it);
  }

  EffectTextureProgram program;
  glGenTextures(1, &program.id);
  glBindTexture(GL_TEXTURE_2D, program.id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    texture.width,
    texture.height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    texture.pixels.data()
  );
  glBindTexture(GL_TEXTURE_2D, 0);
  if (!program.id)
    return 0;

  program.width = texture.width;
  program.height = texture.height;
  auto [inserted, _] = cache.textures.emplace(key, std::move(program));
  return inserted->second.id;
}

void effect_textures_prune(Renderer* renderer) {
  if (!renderer || !renderer->effect_textures)
    return;
  auto& textures = renderer->effect_textures->textures;
  for (auto it = textures.begin(); it != textures.end();) {
    if (!it->second.texture.expired()) {
      ++it;
      continue;
    }
    effect_texture_destroy(it->second);
    it = textures.erase(it);
  }
}

void effect_textures_finish(Renderer* renderer) {
  if (!renderer || !renderer->effect_textures)
    return;
  for (auto& [_, texture] : renderer->effect_textures->textures)
    effect_texture_destroy(texture);
  delete renderer->effect_textures;
  renderer->effect_textures = nullptr;
}

} // namespace ura::render::raylib
