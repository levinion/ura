#include "ura/render/raylib/renderer.hpp"

#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" {
#include <wlr/util/log.h>
}

namespace ura::render::raylib {

static const char* vertex_shader_source = R"(
#version 100
precision highp float;
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform mat4 mvp;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

static const char* texture_vertex_source = R"(
#version 100
precision highp float;
attribute vec4 vertexPositionTexCoord;
varying vec2 fragTexCoord;
uniform mat4 mvp;
void main() {
    fragTexCoord = vertexPositionTexCoord.zw;
    gl_Position = mvp * vec4(vertexPositionTexCoord.xy, 0.0, 1.0);
}
)";

static const char* rect_fragment_source = R"(
#version 100
precision mediump float;
uniform vec4 colDiffuse;
void main() {
    gl_FragColor = colDiffuse;
}
)";

static const char* texture_fragment_source = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
void main() {
    gl_FragColor = texture2D(texture0, fragTexCoord) * colDiffuse;
}
)";

static const char* texture_rgbx_fragment_source = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
void main() {
    vec4 texel = texture2D(texture0, fragTexCoord);
    gl_FragColor = vec4(texel.rgb, 1.0) * colDiffuse;
}
)";

static const char* external_vertex_source = R"(
#version 100
precision highp float;
attribute vec4 vertexPositionTexCoord;
varying vec2 fragTexCoord;
uniform mat4 mvp;
void main() {
    fragTexCoord = vertexPositionTexCoord.zw;
    gl_Position = mvp * vec4(vertexPositionTexCoord.xy, 0.0, 1.0);
}
)";

static const char* effect_fragment_source = R"(
#version 100
precision mediump float;
varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 tintColor;
void main() {
    vec4 texel = texture2D(texture0, fragTexCoord);
    gl_FragColor = tintColor + (texel + fragColor + colDiffuse) * 0.0001;
}
)";

static const char* external_fragment_source = R"(
#version 100
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2 fragTexCoord;
uniform samplerExternalOES texture0;
uniform vec4 colDiffuse;
void main() {
    gl_FragColor = texture2D(texture0, fragTexCoord) * colDiffuse;
}
)";

static void init_shader_locations(Shader* shader) {
  for (int& loc : shader->locs) loc = -1;
}

static GLuint compile_shader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  GLint ok = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      auto log = static_cast<char*>(malloc(static_cast<size_t>(length)));
      if (log) {
        glGetShaderInfoLog(shader, length, nullptr, log);
        wlr_log(WLR_ERROR, "Raylib renderer shader compile failed: %s", log);
        free(log);
      }
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static Shader
load_shader(const char* vs, const char* fs, bool require_texture) {
  Shader shader {};
  init_shader_locations(&shader);

  GLuint vertex = compile_shader(GL_VERTEX_SHADER, vs);
  GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fs);
  if (!vertex || !fragment) {
    if (vertex)
      glDeleteShader(vertex);
    if (fragment)
      glDeleteShader(fragment);
    return shader;
  }

  shader.id = glCreateProgram();
  glAttachShader(shader.id, vertex);
  glAttachShader(shader.id, fragment);
  glBindAttribLocation(shader.id, 0, "vertexPosition");
  glBindAttribLocation(shader.id, 0, "vertexPositionTexCoord");
  glBindAttribLocation(shader.id, 1, "vertexTexCoord");
  glBindAttribLocation(shader.id, 3, "vertexColor");
  glLinkProgram(shader.id);
  glDetachShader(shader.id, vertex);
  glDetachShader(shader.id, fragment);
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  GLint linked = GL_FALSE;
  glGetProgramiv(shader.id, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint length = 0;
    glGetProgramiv(shader.id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
      auto log = static_cast<char*>(malloc(static_cast<size_t>(length)));
      if (log) {
        glGetProgramInfoLog(shader.id, length, nullptr, log);
        wlr_log(WLR_ERROR, "Raylib renderer shader link failed: %s", log);
        free(log);
      }
    }
    glDeleteProgram(shader.id);
    shader.id = 0;
    return shader;
  }

  shader.mvp = glGetUniformLocation(shader.id, "mvp");
  shader.texture = glGetUniformLocation(shader.id, "texture0");
  shader.color = glGetUniformLocation(shader.id, "colDiffuse");
  shader.tint = glGetUniformLocation(shader.id, "tintColor");
  shader.pos_attrib = glGetAttribLocation(shader.id, "vertexPosition");
  if (shader.pos_attrib < 0)
    shader.pos_attrib =
      glGetAttribLocation(shader.id, "vertexPositionTexCoord");
  shader.tex_attrib = glGetAttribLocation(shader.id, "vertexTexCoord");

  shader.locs[RL_SHADER_LOC_VERTEX_POSITION] = shader.pos_attrib;
  shader.locs[RL_SHADER_LOC_VERTEX_TEXCOORD01] = shader.tex_attrib;
  shader.locs[RL_SHADER_LOC_VERTEX_COLOR] =
    glGetAttribLocation(shader.id, "vertexColor");
  shader.locs[RL_SHADER_LOC_MATRIX_MVP] = shader.mvp;
  shader.locs[RL_SHADER_LOC_COLOR_DIFFUSE] = shader.color;
  shader.locs[RL_SHADER_LOC_MAP_DIFFUSE] = shader.texture;

  shader.valid = shader.mvp >= 0 && shader.color >= 0 && shader.pos_attrib >= 0
    && (!require_texture || shader.texture >= 0);
  if (!shader.valid) {
    wlr_log(WLR_ERROR, "Raylib renderer shader is missing required inputs");
    glDeleteProgram(shader.id);
    shader.id = 0;
  }
  return shader;
}

bool shader_init(Renderer* renderer) {
  memset(&renderer->rect, 0, sizeof(renderer->rect));
  memset(&renderer->tex_rgba, 0, sizeof(renderer->tex_rgba));
  memset(&renderer->tex_rgbx, 0, sizeof(renderer->tex_rgbx));
  memset(&renderer->external, 0, sizeof(renderer->external));
  memset(&renderer->effect, 0, sizeof(renderer->effect));

  renderer->rect =
    load_shader(vertex_shader_source, rect_fragment_source, false);
  renderer->effect =
    load_shader(vertex_shader_source, effect_fragment_source, true);
  // rlgl's non-VAO batch path binds every standard attribute location. Keep
  // unused ones valid for the optional effect pass.
  renderer->effect.locs[RL_SHADER_LOC_VERTEX_NORMAL] =
    RL_DEFAULT_SHADER_ATTRIB_LOCATION_NORMAL;
  renderer->effect.locs[RL_SHADER_LOC_VERTEX_TANGENT] =
    RL_DEFAULT_SHADER_ATTRIB_LOCATION_TANGENT;
  renderer->effect.locs[RL_SHADER_LOC_VERTEX_TEXCOORD02] =
    RL_DEFAULT_SHADER_ATTRIB_LOCATION_TEXCOORD2;
  renderer->tex_rgba =
    load_shader(texture_vertex_source, texture_fragment_source, true);
  renderer->tex_rgbx =
    load_shader(texture_vertex_source, texture_rgbx_fragment_source, true);

  if (
    !renderer->rect.valid || !renderer->effect.valid
    || renderer->effect.tint < 0 || !renderer->tex_rgba.valid
    || !renderer->tex_rgbx.valid
  ) {
    wlr_log(WLR_ERROR, "Failed to load raylib renderer shaders");
    shader_finish(renderer);
    return false;
  }

  if (renderer->egl.has_egl_image_external) {
    renderer->external =
      load_shader(external_vertex_source, external_fragment_source, true);
    if (!renderer->external.valid)
      wlr_log(WLR_INFO, "External OES textures will not be available");
  }

  return true;
}

void shader_finish(Renderer* renderer) {
  if (renderer->rlgl_ready)
    rlDrawRenderBatchActive();

  for (Shader* shader : { &renderer->rect,
                          &renderer->effect,
                          &renderer->tex_rgba,
                          &renderer->tex_rgbx,
                          &renderer->external }) {
    if (shader->id) {
      glDeleteProgram(shader->id);
      shader->id = 0;
      shader->valid = false;
    }
  }
}

Matrix shader_ortho_matrix(int width, int height) {
  Matrix mat {};
  mat.m0 = 2.0f / static_cast<float>(width);
  mat.m5 = 2.0f / static_cast<float>(height);
  mat.m10 = 1.0f;
  mat.m12 = -1.0f;
  mat.m13 = -1.0f;
  mat.m15 = 1.0f;
  return mat;
}

void shader_set_projection(int location, int width, int height) {
  Matrix mat = shader_ortho_matrix(width, height);
  // raylib's Matrix fields are grouped by row in memory. OpenGL expects a
  // column-major float[16], so explicitly select the m0..m15 field order.
  const float values[16] = {
    mat.m0, mat.m1, mat.m2,  mat.m3,  mat.m4,  mat.m5,  mat.m6,  mat.m7,
    mat.m8, mat.m9, mat.m10, mat.m11, mat.m12, mat.m13, mat.m14, mat.m15,
  };
  glUniformMatrix4fv(location, 1, GL_FALSE, values);
}

} // namespace ura::render::raylib
