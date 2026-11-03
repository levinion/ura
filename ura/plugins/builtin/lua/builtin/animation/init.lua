local M = {}

M._definitions = {}

local function merge(dst, src)
  for key, value in pairs(src) do
    if
      type(value) == "table"
      and type(dst[key]) == "table"
      and key ~= "easing"
      and key ~= "properties"
    then
      local nested = {}
      for k, v in pairs(dst[key]) do
        nested[k] = v
      end
      merge(nested, value)
      dst[key] = nested
    else
      dst[key] = value
    end
  end
end

---@param name string
---@param spec table
function M.define(name, spec)
  assert(type(name) == "string", "animation name must be a string")
  assert(type(spec) == "table", "animation definition must be a table")
  M._definitions[name] = spec
end

---@param name string
function M.undefine(name)
  M._definitions[name] = nil
end

---@param name string
---@return table|nil
function M.get(name)
  return M._definitions[name]
end

---@param function_ fun(t: number): number
---@return table<number>
function M.sample_easing(function_)
  local samples = {}
  local count = 256
  for i = 0, count - 1 do
    local t = i / (count - 1)
    local ok, value = pcall(function_, t)
    if ok and type(value) == "number" then
      samples[#samples + 1] = value
    else
      -- Fall back to linear for a broken sample instead of failing the whole
      -- animation at runtime.
      samples[#samples + 1] = t
    end
  end
  return samples
end

--- Resolve the policy for one animation start. This is called once per
--- transition, never from the per-frame animation path.
---@param name string
---@param event table|nil
---@param fallback_enabled boolean|nil
---@return table
function M.resolve(name, event, fallback_enabled)
  event = event or {}
  event.kind = event.kind or name

  local spec = {
    enabled = fallback_enabled ~= false,
    duration = ura.opt.animation_duration or 200,
    easing = ura.opt.animation_easing or "in_out_cubic",
  }

  local definition = M._definitions[name]
  if definition then
    merge(spec, definition)
    if definition.enabled == nil then
      spec.enabled = true
    end
  end

  -- Explicit per-call options win over the registered definition.
  if event.opt then
    merge(spec, event.opt)
  end

  -- Hooks run last so applications can override the policy for a single
  -- window or app_id.
  for _, result in ipairs(ura.hook.collect("window-animation-start", event)) do
    if result == false then
      spec.enabled = false
    elseif type(result) == "table" then
      merge(spec, result)
      if result.enabled == nil then
        spec.enabled = true
      end
    end
  end

  if spec.enabled == false then
    return spec
  end

  if type(spec.easing) == "function" then
    spec.easing = M.sample_easing(spec.easing)
  end

  -- Flatten the declarative from/to forms for the low-level C++ APIs.
  if type(spec.from) == "table" then
    spec.from_scale = spec.from.scale or spec.from_scale
    spec.from_opacity = spec.from.opacity or spec.from_opacity
  end
  if type(spec.to) == "table" then
    spec.to_scale = spec.to.scale or spec.to_scale
    spec.to_opacity = spec.to.opacity or spec.to_opacity
  end

  spec.duration = math.max(0, tonumber(spec.duration) or 0)
  return spec
end

ura.animation = M

--- Install the optional open/close hook wiring. Definitions are still
--- user-provided through ura.animation.define().
---@param opt table|nil
function M.setup(opt)
  opt = opt or {}

  if opt.open ~= false then
    ura.hook.add("window-map", function(event)
      if event.initial ~= true then
        return
      end
      local win = ura.class.UraWindow:new(event.id)
      local spec = M.resolve("window-open", {
        id = event.id,
        app_id = win:app_id(),
        title = win:title(),
        kind = "window-open",
      }, false)
      if spec.enabled and (spec.duration or 0) > 0 then
        ura.api.animate_window_open(
          event.id,
          spec.duration,
          spec.easing,
          spec
        )
      end
    end, { ns = "builtin.animation", priority = ura.g.priority.instant })
  end

  if opt.close ~= false then
    ura.hook.add("window-close", function(event)
      local win = ura.class.UraWindow:new(event.id)
      local spec = M.resolve("window-close", {
        id = event.id,
        app_id = win:app_id(),
        title = win:title(),
        kind = "window-close",
      }, false)
      if spec.enabled and (spec.duration or 0) > 0 then
        ura.api.animate_window_close(
          event.id,
          spec.duration,
          spec.easing,
          spec
        )
      end
    end, { ns = "builtin.animation", priority = ura.g.priority.instant })
  end
end

return M
