-- Minimal stubs so the policy module can be tested without a running
-- compositor.
ura = {
  opt = {},
  hook = {
    collect = function()
      return {}
    end,
  },
}

local chunk = assert(loadfile("ura/plugins/builtin/lua/builtin/animation/init.lua"))
local animation = chunk()
assert(animation)

ura.opt.animation_duration = 123
ura.opt.animation_easing = "out_quad"
local move = animation.resolve("window-move", {}, true)
assert(move.enabled == true)
assert(move.duration == 123)
assert(move.easing == "out_quad")

animation.define("window-open", {
  duration = 180,
  easing = function(t)
    return t * t
  end,
  from = { scale = 0.5, opacity = 0.0 },
  to = { scale = 1.0, opacity = 1.0 },
})
local open = animation.resolve("window-open", {}, false)
assert(open.enabled == true)
assert(open.duration == 180)
assert(type(open.easing) == "table" and #open.easing == 256)
assert(open.from.scale == 0.5 and open.to.opacity == 1.0)
assert(open.from_scale == 0.5 and open.to_opacity == 1.0)

-- Explicit per-call options override the definition before hooks run.
local explicit = animation.resolve("window-open", {
  opt = { duration = 40, easing = "linear" },
}, false)
assert(explicit.duration == 40 and explicit.easing == "linear")

-- Hooks are merged in priority order and can disable the animation.
ura.hook.collect = function()
  return {
    { duration = 70, easing = "in_quad" },
    { enabled = false },
  }
end
local disabled = animation.resolve("window-move", {}, true)
assert(disabled.enabled == false)

-- sample_easing clamps broken callbacks to a linear sample instead of
-- aborting the animation.
local samples = animation.sample_easing(function(t)
  if t == 0.5 then
    error("boom")
  end
  return t
end)
assert(#samples == 256 and samples[1] == 0)
assert(math.abs(samples[129] - 0.5) < 0.01)
