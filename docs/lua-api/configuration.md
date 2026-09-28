# Lua API: configuration

## Keybindings

Register one or more key or mouse patterns with `ura.keymap.set`:

```lua
ura.keymap.set({ "super+t", "ctrl+Return" }, function()
  ura.api.spawn("foot")
end)

-- Optional keymap mode (the default mode is "normal"):
ura.keymap.set({ "super+r" }, function()
  ura.cmd.reload()
end, { mode = "normal" })
```

Patterns are case-insensitive and consist of optional `+`-separated modifiers followed by one key. Supported modifiers are `super`, `alt`, `ctrl`, `shift`, `caps`, `mod2`, `mod3`, and `mod5`. Mouse patterns include `mouseleft`, `mouseright`, `mousemiddle`, `mouseside`, `mouseextra`, `wheelup`, `wheeldown`, `wheelleft`, and `wheelright`. Invalid patterns are ignored.

`ura.keymap.unset(patterns, { mode = "normal" })` removes bindings for a mode. `ura.keymap.set_mode(name)` changes the active mode. A binding is selected by both key ID and active mode; ensure a key is bound in each mode in which it should work. The first callback registered for a key/mode pair is retained; a later `set` does not replace it.

## Options: `ura.opt`

`ura.opt` starts as an ordinary empty Lua table. Set values directly in the config, and read them in config/plugin code. Options are not validated against a schema. The following keys are read by the current core or shipped layout plugin; defaults are fallbacks used when the key is absent:

| Option | Type | Default | Effect |
| --- | --- | --- | --- |
| `default_output_tags` | `string[]` | `{ ":1" }` | Initial tags assigned to each output. |
| `focus_follow_mouse` | `boolean` | `true` | Focus the window under the pointer. |
| `unfocus_on_leave` | `boolean` | `false` | Unfocus when the pointer leaves all surfaces while focus-follows-mouse is enabled. |
| `focus_delay` | `number` (ms) | `5` | Delay before pointer-driven focus; non-positive values focus immediately. |
| `default_layout` | `string` | `"tiling"` | Layout selected when toggling off the currently active layout. |
| `animation_duration` | `number` (ms) | `200` | Default duration for `UraWindow` move/resize/opacity animations. |
| `animation_fps` | `number` | `60` | Default update rate for those animations. |

Example:

```lua
ura.opt.default_output_tags = { "work:1" }
ura.opt.animation_duration = 150
ura.opt.focus_follow_mouse = false
```

## Constants: `ura.g`

These tables and `ura.g` itself are read-only proxies.

`ura.g.priority` values (lower runs earlier for hooks):

| Name | Value | Name | Value |
| --- | ---: | --- | ---: |
| `instant` | 0 | `urgent` | 10 |
| `ultra_fast` | 20 | `very_fast` | 30 |
| `fast` | 40 | `normal` | 50 |
| `slow` | 60 | `very_slow` | 70 |
| `ultra_slow` | 80 | `slowest` | 90 |

`ura.g.layer` scene-layer values:

| Name | Value | Name | Value |
| --- | ---: | --- | ---: |
| `clear` | -50 | `background` | 0 |
| `bottom` | 50 | `normal` | 100 |
| `top` | 150 | `floating` | 200 |
| `fullscreen` | 250 | `popup` | 300 |
| `overlay` | 350 | `lock_screen` | 400 |

Use these constants rather than hard-coding built-in priorities or scene layers:

```lua
ura.hook.add("window-layout-change", on_layout, {
  ns = "my-layout",
  priority = ura.g.priority.slow,
})
```

## Built-in commands: `ura.cmd`

- `focus_left()`, `focus_right()`, `focus_up()`, `focus_down()` move focus among windows on the current output. A window with `userdata().focus_exclusive == true` is not moved by these commands.
- `reload()` reloads the selected config file and restores the saved runtime context before applying it. A load or runtime error is raised as a Lua error.

## Loading plugins and config files

At startup, Ura adds built-in, system, and user plugin directories to Lua's `package.path`, then loads the selected config. The shipped config calls `require("builtin.layout").setup()` before registering bindings and hooks. Additional module directories can be added with `ura.fn.load(path)`; use `require` to load a module after its search patterns are available. See [Lua helpers](functions.md#urafn) and [config path selection](../lua-api.md#configuration-loading).
