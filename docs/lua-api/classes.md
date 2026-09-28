# Lua API: object classes

The wrappers in `ura.class` hold opaque compositor IDs and call the matching `ura.api` functions. Use Lua's colon syntax (`:`) for wrapper methods. Comparing two wrappers of the same class compares their IDs (or labels for blocks/segments).

Geometry values use `{ x, y, width, height }`. Window/output coordinates are in the compositor's logical layout, not physical pixels.

## `UraWindow`

Create wrappers with `ura.class.UraWindow:new(id)`. Static-style methods are also called with `:`:

| Method | Description |
| --- | --- |
| `UraWindow:current()` | Focused window wrapper, or `nil`. |
| `UraWindow:all()` | Array of all window wrappers. |
| `UraWindow:from_tags(tags)` | Windows with at least one tag in `tags`. |
| `UraWindow:from_pos(x, y)` | Mapped windows whose geometry contains the logical point. |

For a wrapper `window`:

| Method | Description |
| --- | --- |
| `close()` / `focus()` / `activate()` | Close, focus, or activate the window. |
| `output()` | Its `UraOutput` wrapper, or `nil`. |
| `app_id()` / `title()` | Client app ID/title. |
| `geometry()` | `{ x, y, width, height }`, or `nil`. |
| `z_index()` / `set_z_index(z)` | Get or set scene-layer index. See `ura.g.layer`. |
| `is_fullscreen()` / `is_fullscreen_requested()` / `set_fullscreen(flag)` | Current state, client request state, and setter. |
| `is_maximized()` / `is_maximized_requested()` / `set_maximized(flag)` | Current state, client request state, and setter. |
| `is_resizing()` / `set_resizing(flag)` | Read or set resizing state. |
| `is_mapped()` / `is_focused()` | Read current mapped/focused state. |
| `move(x, y, options)` | Move to logical coordinates. |
| `resize(width, height, options)` | Resize the window. |
| `center(options)` | Center in the output's usable geometry. |
| `opacity()` / `set_opacity(value, options)` | Read or set opacity in `[0, 1]`. |
| `border_color()` / `set_border_color(color)` | Read or set `#RRGGBB` / `#RRGGBBAA`. |
| `lru()` | Focus-order value, useful for ordering recent windows. |
| `tags()` / `set_tags(tags)` | Read or replace tags. `set_tags` naturally sorts the input array. |
| `userdata()` / `set_userdata(table)` / `update_userdata(callback)` | Read, replace, or mutate the window's in-memory Lua user data. |

`move`, `resize`, and `set_opacity` accept an optional `{ duration = milliseconds, fps = number }` table. Defaults are `ura.opt.animation_duration` or `200` ms and `ura.opt.animation_fps` or `60`. A non-positive duration applies immediately. A new animation cancels the previous animation of the same property. `resize` toggles the resizing state during an animation.

## `UraOutput`

Construct with `ura.class.UraOutput:new(id)`.

| Method | Description |
| --- | --- |
| `UraOutput:current()` | Current output wrapper, or `nil`. |
| `UraOutput:all()` | Array of output wrappers. |
| `UraOutput:from_name(name)` | Find by output name, or `nil`. |
| `UraOutput:from_pos(x, y)` | Outputs whose logical geometry contains the point. |
| `name()` / `scale()` | Output name or scale. |
| `logical_geometry()` / `usable_geometry()` | Full logical bounds or usable area. |
| `tags()` / `set_tags(tags)` | Read or replace tags; setting tags naturally sorts them and may map/unmap windows. |
| `select(pattern)` | Set tags to those matching a shell-style glob against the collected tag list. |
| `userdata()` / `set_userdata(table)` / `update_userdata(callback)` | Read, replace, or mutate in-memory user data. |

`UraOutput:set_dpms(flag)` exists in the Lua wrapper, but currently calls `ura.api.set_output_dpms`, which is not registered by the Lua runtime. Treat it as unavailable until that binding is added.

## `UraBlock`

A block is represented by a tag of the form `label:index`, where the final colon-delimited component is the numeric index. Labels may contain colons.

| Method | Description |
| --- | --- |
| `UraBlock:new(label, index)` | Construct a block value. |
| `UraBlock:from_tag(tag)` | Parse a tag; returns `nil` if its final component is not numeric. |
| `UraBlock:current()` | Blocks represented by the current output's tags. |
| `UraBlock:all()` | All parseable blocks found in window/output tags, naturally sorted. |
| `tag()` | Serialize as `label:index`. |
| `segment()` | Convert to the corresponding `UraSegment`. |
| `windows()` | Windows tagged with this block. |

## `UraSegment`

A segment groups blocks that share a label.

| Method | Description |
| --- | --- |
| `UraSegment:new(label)` | Construct a segment value. |
| `UraSegment:from_tag(tag)` / `UraSegment:from_block(block)` | Convert a tag or block to its segment; malformed tags return `nil`. |
| `UraSegment:all()` | Unique segments found in collected tags, naturally sorted. |
| `UraSegment:current()` | Unique segments represented by current output tags. |
| `blocks()` / `windows()` | Blocks or windows belonging to this segment. |
| `active_block()` | The only block if there is one; otherwise the block containing the most-recently-used window, or `nil` if none can be selected. |

## Built-in layout extension

The shipped config calls `require("builtin.layout").setup()`. This adds the following methods to `UraWindow` and registers layout hooks:

| Method | Description |
| --- | --- |
| `layout()` | Current layout name stored in window user data, or `nil`. |
| `set_layout(name)` | Change layout and emit `window-layout-change` with `{ id, from, to }`. |
| `toggle_layout(name)` | Toggle to `name`, or back to `ura.opt.default_layout` (default `"tiling"`) when already active. |
| `shrink(ratio)` / `expand(ratio)` | Adjust the current tiling window's weight and reapply the tiling layout. |

The default setup enables `floating`, `fullscreen`, `tiling`, and `maximize` layouts. Pass `{ floating = false }` (or the corresponding layout name) to disable one. Tiling options are `outer_l`, `outer_r`, `outer_t`, `outer_b`, and `inner` (each defaults to `10`); maximize accepts the four `outer_*` margins (also default `10`). For example:

```lua
require("builtin.layout").setup({
  tiling = { inner = 8 },
  maximize = { outer_l = 16, outer_r = 16 },
})
```
