#pragma once

#include <array>
#include <cstdint>

namespace ura {

enum class EffectType {
  None,
  // Separable two-pass Gaussian blur applied through offscreen render targets.
  Blur,
  // Multiplicative color tint.
  Tint,
  // Alpha mask/tint. Uses the alpha channel of `color`.
  Mask,
};

struct Effect {
  EffectType type = EffectType::None;
  // Blur radius in output pixels.
  float radius = 8.f;
  // Premultiplied tint/mask color.
  std::array<float, 4> color { { 1.f, 1.f, 1.f, 1.f } };
  // Reserved for a custom renderer-side shader resource.
  uint64_t shader = 0;

  bool active() const {
    return this->type != EffectType::None;
  }

  bool operator==(const Effect&) const = default;
};

} // namespace ura
