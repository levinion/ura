local M = {}

local effect = {}

function effect.define(spec)
  return ura.api.define_effect(spec)
end

function effect.clear()
  ura.api.clear_effects()
end

function effect.texture(spec)
  local path = type(spec) == "string" and spec or (spec and spec.path)
  if type(path) ~= "string" then
    return nil
  end
  return ura.api.define_texture(path)
end

function effect.texture_get(name)
  return ura.api.find_texture(name)
end

function effect.texture_undefine(value)
  if type(value) ~= "table" or not value.id then
    return false
  end
  return ura.api.undefine_texture(value.id, value.generation or 1)
end

function effect.get(name)
  return ura.api.find_effect(name)
end

function effect.undefine(value)
  if type(value) ~= "table" or not value.id then
    return false
  end
  return ura.api.undefine_effect(value.id, value.generation or 1)
end

setmetatable(effect, {
  __call = function(_, spec)
    return effect.define(spec)
  end,
})

M.effect = effect

return M
