#include "ura/render/effect_registry.hpp"

#include <iostream>

int main() {
  ura::EffectRegistry registry;

  ura::EffectPipeline first;
  first.name = "named";
  auto first_handle = registry.define(first);
  if (!registry.get(first_handle) || !registry.find("named"))
    return 1;

  ura::EffectPipeline replacement;
  replacement.name = "named";
  auto replacement_handle = registry.define(replacement);
  if (registry.get(first_handle))
    return 1;
  auto found = registry.find("named");
  if (!found || found->handle != replacement_handle)
    return 1;

  if (!registry.remove(replacement_handle) || registry.find("named"))
    return 1;
  if (registry.remove(replacement_handle))
    return 1;

  ura::EffectTexture image;
  image.name = "noise";
  image.width = 1;
  image.height = 1;
  image.pixels = { 0, 255, 0, 255 };
  auto image_handle = registry.define_texture(image);
  if (!registry.get_texture(image_handle) || !registry.find_texture("noise"))
    return 1;
  if (!registry.remove_texture(image_handle) || registry.find_texture("noise"))
    return 1;

  auto other = registry.define(
    ura::EffectPipeline {
      .name = "other",
      .passes = {},
      .uniforms = {},
      .max_outset = {},
    }
  );
  if (!registry.get(other) || !registry.find("other"))
    return 1;
  registry.clear();
  if (registry.find("other") || registry.get(other))
    return 1;

  std::cout << "effect registry: ok\n";
  return 0;
}
