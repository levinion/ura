# Lua API

This reference documents the Lua surface currently loaded by Ura. The global `ura` table is created when the runtime loads; configuration and plugin files can use its modules directly. Lua code runs in LuaJIT and the standard libraries opened by Ura, including `ffi`, `io`, `os`, `package`, `math`, `string`, and `table`.

## Reference pages

- [Configuration, keybindings, options, and constants](lua-api/configuration.md)
- [Hooks and event payloads](lua-api/hooks.md)
- [Object wrappers: windows, outputs, blocks, and segments](lua-api/classes.md)
- [Low-level `ura.api` functions and `ura.fn` helpers](lua-api/functions.md)

## `ura` modules

| Module | Purpose |
| --- | --- |
| `ura.api` | Low-level compositor operations and utility functions. |
| `ura.class` | Object wrappers such as `UraWindow` and `UraOutput`. |
| `ura.hook` | Register, remove, or emit Lua hooks. |
| `ura.keymap` | Bind key or mouse patterns to Lua callbacks. |
| `ura.opt` | User-defined configuration options; it starts as an empty table. |
| `ura.fn` | Lua helpers for tables, tags, paths, and configuration loading. |
| `ura.cmd` | Built-in commands, including directional focus and config reload. |
| `ura.g` | Read-only priority and scene-layer constants. |

The built-in layout plugin extends `UraWindow` with layout helpers. Those methods are available when `builtin.layout` is loaded and set up (as in the shipped `assets/init.lua`). See [classes](lua-api/classes.md#built-in-layout-extension).

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
