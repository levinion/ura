# Lua API: animation

`ura.animation` is an optional built-in plugin:

```lua
require("builtin.animation").setup()
```

It defines window animation policy in Lua. Policy is resolved once when an
animation starts; the C++ animation system samples the resulting easing LUT and
updates `SceneNode.bounds` / `Opacity` every frame without calling Lua.
Without the plugin, `UraWindow:move`, `resize`, and `set_opacity` only animate
when the caller passes an explicit `duration`.

## `ura.animation.define(name, spec)`

Register or replace a named animation definition.

```lua
ura.animation.define("window-resize", {
  duration = 220,
  easing = "in_out_cubic",
  properties = { "position", "scale" },
})

ura.animation.define("window-open", {
  duration = 180,
  easing = "out_cubic",
  from = { scale = 0.9, opacity = 0.0 },
  to = { scale = 1.0, opacity = 1.0 },
})

ura.animation.define("window-close", {
  duration = 140,
  easing = "in_quad",
  to = { scale = 0.94, opacity = 0.0 },
})
```

Recognized keys:

| Key | Description |
| --- | --- |
| `duration` | Duration in milliseconds. Non-positive disables the animation. |
| `easing` | Built-in name, LUT array, or pure Lua function `f(t) -> number`. |
| `enabled` | Set to `false` to disable the animation. |
| `from.scale` / `to.scale` | Scale relative to the current `SceneNode.bounds`, around its center. |
| `from.opacity` / `to.opacity` | Opacity endpoints. Used by `window-open`, `window-close`, and `window-opacity`. |
| `properties` | Reserved for future property filtering. |

The built-in names include `linear`, `in_quad`, `out_quad`, `in_out_quad`,
`in_cubic`, `out_cubic`, `in_out_cubic`, `in_quart`, `out_quart`,
`in_out_quart`, `in_expo`, `out_expo`, `in_out_expo`, `out_back`, and
`in_out_back`.

A Lua easing function is sampled into a 256-entry LUT at animation start:

```lua
ura.animation.define("window-move", {
  duration = 260,
  easing = function(t)
    return 1 - (1 - t) ^ 4
  end,
})
```

## `ura.animation.resolve(name, event, fallback_enabled)`

Resolve the rules for one animation start. `UraWindow:move`, `resize`, and
`set_opacity` call this automatically. The precedence is:

1. defaults (`ura.opt.animation_duration`, `ura.opt.animation_easing`);
2. the registered definition;
3. explicit per-call options (`duration`, `easing`, `enabled`);
4. `window-animation-start` hook results, in hook priority order.

If only `window-move` is defined, it is used as the fallback timing for a
combined layout resize+move. Define `window-resize` explicitly when resize-only
changes should animate too.

`window-open` and `window-close` are handled by the plugin's
`window-map` / `window-close` hooks with `fallback_enabled = false`. They only
run when a definition or hook enables them. The low-level APIs are:

```lua
ura.api.animate_window_open(id, duration, easing, {
  from_scale = 0.92,
  from_opacity = 0.0,
  to_scale = 1.0,
  to_opacity = 1.0,
})

ura.api.animate_window_close(id, duration, easing, {
  to_scale = 0.94,
  to_opacity = 0.0,
})
```

Example app-specific override:

```lua
ura.hook.add("window-animation-start", function(event)
  if event.app_id == "kitty" then
    return { duration = 120, easing = "out_quart" }
  end
end)
```

Return `{ enabled = false }` to disable one animation without removing its
definition.

## Helpers

| Function | Description |
| --- | --- |
| `ura.animation.get(name)` | Return the registered definition, or `nil`. |
| `ura.animation.undefine(name)` | Remove a definition. |
| `ura.animation.sample_easing(fn)` | Sample a Lua function into a 256-entry LUT. |

## Related low-level functions

`ura.api.animate_window_move`, `animate_window_resize`, and
`animate_window_opacity` accept either an easing name or a LUT table. They are
normally called through `UraWindow` methods, which resolve `ura.animation`
policy first.
