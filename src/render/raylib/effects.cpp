#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>

#include "ura/util/rgb.hpp"

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

// Optional post-processing hook, disabled by default:
//
//   URA_RAYLIB_EFFECT_TINT=#RRGGBBAA ura
//
// This path deliberately goes through rlgl's immediate-mode batch. Keeping it
// out of the wlr_scene hot path avoids rlgl's global batch state interacting
// with the direct-GL renderer, while still providing a real rlgl integration
// point for custom effects.
void effects_apply(Renderer* renderer, RenderPass* pass) {
  if (!renderer || !pass || !renderer->rlgl_ready || !renderer->effect.valid)
    return;

  const char* value = getenv("URA_RAYLIB_EFFECT_TINT");
  if (!value || !*value)
    return;

  auto rgba = ura::util::hex2rgba(value);
  if (!rgba || (*rgba)[3] <= 0.0f)
    return;

  float r = (*rgba)[0] * (*rgba)[3];
  float g = (*rgba)[1] * (*rgba)[3];
  float b = (*rgba)[2] * (*rgba)[3];
  float a = (*rgba)[3];

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_SCISSOR_TEST);
  glScissor(0, 0, pass->width, pass->height);

  rlSetShader(renderer->effect.id, renderer->effect.locs);
  rlSetTexture(renderer->white_texture);
  glUseProgram(renderer->effect.id);
  glUniform4f(renderer->effect.tint, r, g, b, a);

  rlBegin(RL_QUADS);
  rlColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  rlTexCoord2f(0.0f, 0.0f);
  rlVertex3f(0.0f, 0.0f, 0.0f);
  rlTexCoord2f(1.0f, 0.0f);
  rlVertex3f(static_cast<float>(pass->width), 0.0f, 0.0f);
  rlTexCoord2f(1.0f, 1.0f);
  rlVertex3f(
    static_cast<float>(pass->width),
    static_cast<float>(pass->height),
    0.0f
  );
  rlTexCoord2f(0.0f, 1.0f);
  rlVertex3f(0.0f, static_cast<float>(pass->height), 0.0f);
  rlEnd();
  rlDrawRenderBatchActive();

  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);
  glUseProgram(0);
  wlr_log(WLR_DEBUG, "Applied raylib tint effect %s", value);
}

} // namespace ura::render::raylib
