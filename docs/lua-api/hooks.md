# Lua API: hooks

Hooks let configuration and plugins run code at compositor events. Register callbacks with `ura.hook.add(name, callback, options)`:

```lua
local id = ura.hook.add("window-new", function(event)
  local window = ura.class.UraWindow:new(event.id)
  window:set_border_color("#89b4fa")
end, {
  ns = "my-config",
  priority = ura.g.priority.normal,
  desc = "style new windows",
})
```

The callback receives one event table. Hook callbacks run in ascending `priority` order (lower values run first); the default is `50`, and order among equal-priority callbacks is not guaranteed. `ns` defaults to `"default"` and is useful for removing a group of callbacks. `desc` is optional metadata.

```lua
ura.hook.remove({ id = id })
ura.hook.remove({ ns = "my-config" })
```

`remove` accepts an `id`, an `ns`, or both. If both are supplied, a hook is removed if either selector matches. A callback error is caught by the Lua hook dispatcher, is not reported there, and does not stop other callbacks.

## Event hooks

The following names are emitted by the compositor or shipped Lua plugins in the current source tree. Fields marked optional may be absent.

| Hook | Event fields | Timing / behavior |
| --- | --- | --- |
| `prepare` | `{}` | After the Lua runtime and config load, before the Wayland socket and backend are started. Suitable for startup environment setup. |
| `ready` | `{}` | After the backend starts and the Wayland event loop is registered. |
| `new-input` | `{ name }` | An input device is added. |
| `output-new` | `{ id }` | A new output is initialized. |
| `output-resume` | `{ id }` | An output is resumed/restored. |
| `output-usable-geometry-change` | `{ id }` | The output's usable area changes, for example when an exclusive layer surface appears. |
| `output-tags-change` | `{ id, from, to }` | Output tags changed; `from` and `to` are arrays of strings. |
| `window-new` | `{ id }` | A toplevel is prepared. This occurs before its first map, not necessarily after it is focused. |
| `pre-window-map` | `{ id, initial }` | Immediately before a window is mapped. `initial` is true for its initial map. |
| `window-map` | `{ id, initial }` | After the window is mapped. |
| `pre-window-unmap` | `{ id }` | Immediately before a mapped window is unmapped. |
| `window-unmap` | `{ id }` | After the window is unmapped. |
| `window-close` | `{ id }` | The window is closing. Treat the ID as short-lived. |
| `window-focus` / `window-unfocus` | `{ id }` | A window receives or loses keyboard focus. |
| `pre-window-activate` | `{ id }` | Before activating a window. Returning `false` from any callback cancels activation. |
| `window-activate` | `{ id }` | After activation and focus have been applied. |
| `window-move` / `window-resize` | `{ id }` | Window geometry changes. |
| `window-title-change` / `window-app_id-change` | `{ id }` | The corresponding toplevel metadata changes. |
| `window-tags-change` | `{ id, from, to }` | Window tags changed; `from` and `to` are arrays of strings. |
| `window-request-maximize` | `{ id }` | A client requests maximization. The built-in layout plugin handles this by toggling the `maximize` layout. |
| `window-request-fullscreen` | `{ id, requested_fullscreen?, fullscreen?, output_id? }` | A client or foreign-toplevel request changes fullscreen state. The two boolean names are used by different request sources; `output_id` is optional. |
| `keyboard-key` | `{ id, state }` | Keyboard event before keymaps/client delivery. `state` is `"pressed"` or `"released"`; returning `false` suppresses further processing of the event. |
| `mouse-key` | `{ id, state }` | Pointer-button event. `state` is `"pressed"` or `"released"`; returning `false` consumes the event. |
| `mouse-axis` | `{ id }` | Wheel direction represented as a mouse keybinding ID; returning `false` consumes the event. |
| `window-layout-change` | `{ id, from, to }` | Emitted by the built-in layout extension when `UraWindow:set_layout()` changes a window's layout. |

`id` values for windows and outputs are opaque integers; pass them to the corresponding `ura.class` constructor. The event tables are snapshots of the supplied fields—query the wrapper for current object state.

## Custom hooks

`ura.hook.emit(name, event)` can dispatch an application-defined hook using the same callbacks and priority ordering:

```lua
ura.hook.add("my-plugin:refresh", function(event)
  print(event.reason)
end)

ura.hook.emit("my-plugin:refresh", { reason = "config changed" })
```

Only emit names that have at least one registered callback; the current implementation does not treat an unknown name as a no-op.

## Removing callbacks during reload

Use a namespace for each plugin/config module and remove it before re-registering callbacks. The built-in modules follow this convention with namespaces such as `layout.tiling` and `layout.fullscreen`.
