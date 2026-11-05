# Lua API

This reference documents the Lua surface currently loaded by Ura. The global `ura` table is created when the runtime loads; configuration and plugin files can use its modules directly. Lua code runs in LuaJIT and the standard libraries opened by Ura, including `ffi`, `io`, `os`, `package`, `math`, `string`, and `table`.

## Reference pages

- [Configuration, keybindings, options, and constants](lua-api/configuration.md)
- [Hooks and event payloads](lua-api/hooks.md)
- [Animation policy and custom easing](lua-api/animation.md)
- [Idle timeouts and DPMS](lua-api/idle.md)
- [Object wrappers: windows and outputs](lua-api/classes.md)
- [Low-level `ura.api` functions and `ura.fn` helpers](lua-api/functions.md)

## `ura` modules

| Module | Purpose |
| --- | --- |
| `ura.api` | Low-level compositor operations and utility functions. |
| `ura.class` | Object wrappers such as `UraWindow` and `UraOutput`. |
| `ura.hook` | Register, remove, or emit Lua hooks. |
| `ura.animation` | Optional declarative policy from the `builtin.animation` plugin. |
| `ura.render` | Define shader effect pipelines and apply them to windows. |
| `ura.keymap` | Bind key or mouse patterns to Lua callbacks. |
| `ura.opt` | User-defined configuration options; it starts as an empty table. |
| `ura.fn` | Lua helpers for tables, tags, paths, and configuration loading. |
| `ura.cmd` | Built-in commands, including directional focus and config reload. |
| `ura.g` | Read-only priority and scene-layer constants. |

The built-in layout plugin extends `UraWindow` with layout helpers. Those methods are available when `builtin.layout` is loaded and set up (as in the shipped `assets/init.lua`). See [classes](lua-api/classes.md#built-in-layout-extension).

`builtin.effects` is an optional example plugin for multi-pass shader effects. It is not loaded by default; see [window effects](lua-api/classes.md#effects).

## `ext-workspace-v1` and tags

Ura publishes its output tags through `ext-workspace-v1`. Each output becomes
one workspace group; tag names become workspace names, and every currently
active tag is marked active in that group. This means multiple workspaces can be
active at once, matching Ura's multi-tag model.

A normal activate request from a panel such as Waybar replaces that output's
active tag set with the selected tag. Lua can still activate multiple tags:

```lua
local output = ura.class.UraOutput:current()
output:set_tags({ ":1", ":3" })
```

The published workspace set is reconciled from the tags currently used by
outputs and windows. A tag that no output has active and no window carries is
removed from the panel immediately. Active output tags remain visible until
the output changes tag. Tags are canonicalized as `namespace:index`; Lua
setters accept a bare name and append `:1` when no numeric suffix is present.

Ura replaces Lua's global `print(...)` with a buffered implementation; output is returned by the `ura-shell` command after the script completes.

## Configuration loading

The runtime loads its Lua modules, adds the system and user plugin directories to `package.path`, then runs a configuration file. The file selection is:

1. `$XDG_CONFIG_HOME/ura/init.lua`, if `XDG_CONFIG_HOME` is non-empty;
2. otherwise `$HOME/.config/ura/init.lua`;
3. `/etc/ura/init.lua` as the fallback.

If the selected user path does not exist, Ura falls back to `/etc/ura/init.lua`; it does not try `$HOME/.config/ura/init.lua` after a non-empty `XDG_CONFIG_HOME` path. `ura.fn.find_config_path()` implements this selection.

The `prepare` and `ready` hooks bracket compositor startup. See the [hook reference](lua-api/hooks.md) for their timing and all currently emitted events.

## Basic example

```lua
ura.keymap.set({ "super+q" }, function()
  local window = ura.class.UraWindow:current()
  if window then
    window:close()
  end
end)

ura.hook.add("window-new", function(event)
  local window = ura.class.UraWindow:new(event.id)
  window:set_border_color("#89b4fa")
end, { ns = "my-config" })
```

Compositor object IDs are opaque integers. Use them to construct the corresponding wrapper or pass them back to `ura.api`; do not treat them as stable identifiers across object destruction or compositor restarts.
