-- Optional example effect plugin. It is intentionally not loaded by the
-- shipped configuration; load it explicitly with:
--
--   local effects = require("builtin.effects")
--   window:set_effect(effects.blur, { radius = 8 })
--
local M = {}

local blur_shader = [[
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 texelSize;
uniform vec2 direction;
uniform float radius;

void main() {
  vec2 offset = direction * max(radius, 0.0) * texelSize;
  vec4 color = texture2D(texture0, fragTexCoord) * 0.2270270270;
  color += texture2D(texture0, fragTexCoord + offset * 1.3846153846) * 0.3162162162;
  color += texture2D(texture0, fragTexCoord - offset * 1.3846153846) * 0.3162162162;
  color += texture2D(texture0, fragTexCoord + offset * 3.2307692308) * 0.0702702703;
  color += texture2D(texture0, fragTexCoord - offset * 3.2307692308) * 0.0702702703;
  gl_FragColor = color;
}
]]

local tint_shader = [[
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec4 tint;
void main() {
  gl_FragColor = texture2D(texture0, fragTexCoord) * tint;
}
]]

local mask_shader = [[
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float opacity;
void main() {
  vec4 texel = texture2D(texture0, fragTexCoord);
  gl_FragColor = vec4(texel.rgb * opacity, texel.a * opacity);
}
]]

M.tint = ura.render.effect {
  name = "builtin.tint",
  uniforms = {
    tint = "vec4",
  },
  passes = {
    {
      shader = tint_shader,
    },
  },
}

M.mask = ura.render.effect {
  name = "builtin.mask",
  uniforms = {
    opacity = "float",
  },
  passes = {
    {
      shader = mask_shader,
    },
  },
}

M.blur = ura.render.effect {
  name = "builtin.gaussian-blur",
  padding = 32,
  uniforms = {
    radius = "float",
  },
  passes = {
    {
      shader = blur_shader,
      uniforms = {
        direction = { 1.0, 0.0 },
      },
    },
    {
      shader = blur_shader,
      uniforms = {
        direction = { 0.0, 1.0 },
      },
    },
  },
}

function M.setup()
  -- Definitions are created when the module is loaded. Kept for symmetry with
  -- the other builtin plugins.
end

return M
