#include "ura/render/raylib/renderer.hpp"
#include "ura/core/log.hpp"

#include <utility>

namespace ura::render::raylib {

namespace {

  void effect_program_delete(EffectProgram& program) {
    for (auto& pass : program.passes) {
      if (pass.shader.id)
        glDeleteProgram(pass.shader.id);
    }
    program.passes.clear();
  }

  bool effect_program_compile(
    EffectProgram& program,
    const EffectPipeline& pipeline
  ) {
    for (auto& source : pipeline.passes) {
      auto shader = shader_load_fragment(source.fragment_shader.c_str());
      if (!shader.valid)
        return false;

      EffectPassProgram pass {
        .shader = shader,
        .uniforms = {},
        .samplers = {},
        .texel_size = glGetUniformLocation(shader.id, "texelSize"),
      };
      for (auto& uniform : pipeline.uniforms) {
        pass.uniforms[uniform.name] =
          glGetUniformLocation(shader.id, uniform.name.c_str());
      }
      for (auto& uniform : source.uniforms) {
        pass.uniforms[uniform.name] =
          glGetUniformLocation(shader.id, uniform.name.c_str());
      }
      for (auto& input : source.inputs) {
        pass.samplers[input.name] =
          glGetUniformLocation(shader.id, input.name.c_str());
      }
      if (shader.texture >= 0)
        pass.samplers["texture0"] = shader.texture;
      program.passes.push_back(std::move(pass));
    }
    return !program.passes.empty();
  }

} // namespace

EffectProgram*
effect_program_get(Renderer* renderer, const EffectInstance& instance) {
  if (!renderer || !instance.active())
    return nullptr;

  if (!renderer->effect_programs)
    renderer->effect_programs = new EffectProgramCache;

  auto& cache = *renderer->effect_programs;
  auto* key = instance.pipeline.get();
  auto it = cache.programs.find(key);
  if (it != cache.programs.end()) {
    auto existing = it->second.pipeline.lock();
    if (existing.get() == key)
      return it->second.failed ? nullptr : &it->second;
    effect_program_delete(it->second);
    cache.programs.erase(it);
  }

  EffectProgram program;
  program.handle = instance.pipeline->handle;
  program.pipeline = instance.pipeline;
  if (!effect_program_compile(program, *instance.pipeline)) {
    effect_program_delete(program);
    log::error(
      "EffectInstance pipeline '{}' failed to compile; effect disabled",
      instance.pipeline->name
    );
    EffectProgram failed {};
    failed.handle = instance.pipeline->handle;
    failed.pipeline = instance.pipeline;
    failed.failed = true;
    cache.programs.emplace(key, std::move(failed));
    return nullptr;
  }

  auto [inserted, _] = cache.programs.emplace(key, std::move(program));
  return &inserted->second;
}

void effect_programs_prune(Renderer* renderer) {
  if (!renderer || !renderer->effect_programs)
    return;
  auto& programs = renderer->effect_programs->programs;
  for (auto it = programs.begin(); it != programs.end();) {
    if (!it->second.pipeline.expired()) {
      ++it;
      continue;
    }
    effect_program_delete(it->second);
    it = programs.erase(it);
  }
}

void effect_programs_finish(Renderer* renderer) {
  if (!renderer || !renderer->effect_programs)
    return;
  for (auto& [_, program] : renderer->effect_programs->programs)
    effect_program_delete(program);
  delete renderer->effect_programs;
  renderer->effect_programs = nullptr;
}

} // namespace ura::render::raylib
