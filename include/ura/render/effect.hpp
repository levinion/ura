#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ura/util/rect.hpp"

namespace ura {

struct PaintOutset {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;

  bool empty() const {
    return this->left <= 0 && this->top <= 0 && this->right <= 0
      && this->bottom <= 0;
  }

  wlr_box apply(const wlr_box& box) const {
    return {
      .x = box.x - this->left,
      .y = box.y - this->top,
      .width = box.width + this->left + this->right,
      .height = box.height + this->top + this->bottom,
    };
  }

  Rect<int> apply(const Rect<int>& box) const {
    return {
      .x = box.x - this->left,
      .y = box.y - this->top,
      .width = box.width + this->left + this->right,
      .height = box.height + this->top + this->bottom,
    };
  }

  bool operator==(const PaintOutset&) const = default;
};

enum class EffectUniformType {
  Float,
  Int,
  Bool,
  Vec2,
  Vec3,
  Vec4,
};

struct EffectUniformDef {
  std::string name;
  EffectUniformType type = EffectUniformType::Float;

  bool operator==(const EffectUniformDef&) const = default;
};

struct EffectUniform {
  std::string name;
  EffectUniformType type = EffectUniformType::Float;
  std::array<float, 4> values { { 0.f, 0.f, 0.f, 0.f } };

  bool operator==(const EffectUniform&) const = default;
};

struct EffectHandle {
  uint64_t id = 0;
  uint32_t generation = 0;

  explicit operator bool() const {
    return this->id != 0;
  }

  bool operator==(const EffectHandle&) const = default;
};

struct EffectTextureHandle {
  uint64_t id = 0;
  uint32_t generation = 0;

  explicit operator bool() const {
    return this->id != 0;
  }

  bool operator==(const EffectTextureHandle&) const = default;
};

// CPU-side RGBA8 image. The renderer uploads it lazily and keeps only a weak
// reference so removing the registry entry can release the GL texture.
struct EffectTexture {
  EffectTextureHandle handle;
  std::string name;
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;
};

struct EffectPassInput {
  // Shader sampler uniform name, for example "noiseTexture".
  std::string name;
  // "source", "previous", the id of an earlier pass, or "texture:<id>:<gen>".
  std::string source;
  // Non-null for plugin image textures.
  std::shared_ptr<const EffectTexture> texture;

  bool operator==(const EffectPassInput&) const = default;
};

struct EffectPass {
  // Optional output name used by later passes. It must be unique.
  std::string id;
  // GLSL fragment source. The renderer supplies the common vertex shader.
  std::string fragment_shader;
  // Empty means bind the previous pass output to "texture0".
  std::vector<EffectPassInput> inputs;
  // Values fixed for this pass. Instance uniforms are uploaded first and may
  // be overridden by a pass uniform with the same name.
  std::vector<EffectUniform> uniforms;

  bool operator==(const EffectPass&) const = default;
};

struct EffectPipeline {
  EffectHandle handle;
  std::string name;
  std::vector<EffectPass> passes;
  // Per-instance uniform schema.
  std::vector<EffectUniformDef> uniforms;
  // Conservative, logical-pixel expansion used by the frame planner whenever
  // an instance of this pipeline is active.
  PaintOutset max_outset;
};

// Per-window effect state. `pipeline` also keeps the CPU-side descriptor alive
// for as long as a FramePlan can reference this instance.
struct EffectInstance {
  std::shared_ptr<const EffectPipeline> pipeline;
  uint64_t version = 0;
  // Actual bounds expansion for this instance. Defaults to
  // EffectPipeline::max_outset and may be overridden by the caller.
  PaintOutset outset;
  std::vector<EffectUniform> uniforms;

  bool active() const {
    return this->pipeline && this->pipeline->handle.id != 0;
  }

  bool operator==(const EffectInstance&) const = default;
};

} // namespace ura
