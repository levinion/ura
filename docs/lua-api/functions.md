# Lua API: functions

This page covers the low-level functions installed under `ura.api` and the Lua helpers under `ura.fn`. Low-level functions generally return `nil`/do nothing when an object ID is no longer valid; where that behavior matters, it is noted below. IDs are opaque integers, not persistent handles.

## `ura.api`

### Runtime and timers

| Function | Description |
| --- | --- |
| `terminate()` | Request compositor shutdown. |
| `spawn(command)` | Run `command` through `/bin/sh -c` in a child process. Standard output and error are redirected to `/dev/null`. Do not pass untrusted text. |
| `notify(summary, body)` | Send a desktop notification. |
| `set_timeout(callback, milliseconds)` | Invoke `callback` once after the delay. Returns a timer ID, or `nil` if the delay is non-positive (in that case the callback is invoked immediately). |
| `clear_timeout(timer_id)` | Cancel a timeout. |
| `set_interval(callback, milliseconds)` | Invoke `callback` repeatedly at the interval. Returns a timer ID, or `nil` if the interval is non-positive (in that case the callback is invoked immediately). |
| `clear_interval(timer_id)` | Cancel an interval. |
| `notify_idle_activity()` | Report user activity to the idle subsystem. |
| `set_idle_inhibitor(enabled)` | Enable or disable the compositor's idle inhibitor. |

### Windows

`id` is an opaque window ID, usually obtained from a hook's `event.id`, `get_current_window()`, or `get_all_windows()`.

| Function | Description |
| --- | --- |
| `get_current_window()` | Focused window ID, or `nil`. |
| `get_all_windows()` | Array of current window IDs. |
| `get_window_output(id)` | Output ID for the window, or `nil`. |
| `get_window_app_id(id)` / `get_window_title(id)` | App ID or title, or `nil` for an invalid ID. |
| `get_window_geometry(id)` | `{ x, y, width, height }`, or `nil` for an invalid ID. Coordinates are in the compositor's logical layout. |
| `get_window_z_index(id)` / `set_window_z_index(id, z)` | Read or change the window's scene-layer index. |
| `focus_window(id)` / `activate_window(id)` / `close_window(id)` | Focus, activate, or request close for a window. Invalid IDs are ignored. |
| `move_window(id, x, y)` | Move the window to logical coordinates. |
| `resize_window(id, width, height)` | Set the window's size. |
| `is_window_fullscreen(id)` / `is_window_fullscreen_requested(id)` | Current fullscreen state or the client's requested state; `nil` for an invalid ID. |
| `set_window_fullscreen(id, enabled)` | Set fullscreen state. |
| `is_window_maximized(id)` / `is_window_maximized_requested(id)` | Current maximized state or the client's requested state. |
| `set_window_maximized(id, enabled)` | Set maximized state. |
| `is_window_resizing(id)` / `set_window_resizing(id, enabled)` | Read or set the resizing state. |
| `is_window_mapped(id)` / `is_window_focused(id)` | Read mapped or focused state. |
| `get_window_lru(id)` | Monotonic focus-order value, or `nil` for an invalid ID. It is useful for ordering, not as a timestamp. |
| `get_window_opacity(id)` / `set_window_opacity(id, opacity)` | Read or set opacity in the range `[0, 1]`. Out-of-range values and invalid IDs are ignored by the setter. |
| `get_window_border_color(id)` / `set_window_border_color(id, color)` | Read or set an `#RRGGBB` or `#RRGGBBAA` border color. |
| `get_window_tags(id)` / `set_window_tags(id, tags)` | Read or replace the window's array of string tags. |

### Outputs

| Function | Description |
| --- | --- |
| `get_current_output()` | Focused/current output ID, or `nil`. |
| `get_output(name)` | Output ID matching `name`, or `nil`. |
| `get_all_outputs()` | Array of current output IDs. |
| `get_output_name(id)` | Output name, or `nil` for an invalid ID. |
| `get_output_logical_geometry(id)` | `{ x, y, width, height }` in logical layout coordinates. |
| `get_output_usable_geometry(id)` | Usable area after exclusive layer surfaces reserve space, in the same shape. |
| `get_output_scale(id)` | Output scale, or `nil` for an invalid ID. |
| `get_output_tags(id)` / `set_output_tags(id, tags)` | Read or replace the output's array of string tags. Changing output tags can map/unmap windows whose tags no longer match. |

### Input

| Function | Description |
| --- | --- |
| `set_keyboard_repeat(rate, delay)` | Configure keyboard repeat rate and delay. |
| `set_cursor_theme(theme, size)` | Set the X cursor theme and size. |
| `set_cursor_visible(enabled)` / `get_cursor_visible()` | Set or query cursor visibility. |
| `set_cursor_shape(name)` | Set the cursor's X cursor shape by name. |
| `get_cursor_pos()` | Cursor position as `{ x, y }` in logical layout coordinates. |

Only the functions listed here are currently installed in the Lua runtime. Some cursor getters exist in C++ or the LuaLS stub but are not bound under `ura.api`.

### Environment, packages, JSON, and user data

| Function | Description |
| --- | --- |
| `set_env(name, value)` / `unset_env(name)` | Set or remove a process environment variable. |
| `append_package_path(path)` / `prepend_package_path(path)` | Add a Lua `package.path` pattern at the end or beginning. An exact duplicate is not added. |
| `expanduser(path)` | Expand `~` or `~user` at the start of a path. An unknown user leaves the input unchanged. |
| `expandvars(path)` | Expand `$NAME` and `${NAME}`. An unset variable is replaced with an empty string. |
| `expand(path)` | Apply `expandvars` and then `expanduser`. |
| `to_json(value)` | Serialize a Lua value as JSON. Tables become arrays or objects; functions become the string `"<lua function>"`; unsupported values become `null`. |
| `parse_json(text)` | Parse JSON into Lua values. Invalid JSON returns `nil`. Trailing non-whitespace is rejected. |
| `set_userdata(id, value)` / `get_userdata(id)` | Store/retrieve a Lua value associated with a live window or output ID. User data is in-memory and is not persistent across restarts. |

### Keybinding and clock helpers

| Function | Description |
| --- | --- |
| `get_keybinding_id(pattern)` | Convert a key or mouse pattern to the integer ID used by keymaps and input hooks. Returns `nil` for an invalid pattern. See [keybinding patterns](configuration.md#keybindings). |
| `time_since_epoch()` | Return the raw tick count of `steady_clock`. This is monotonic, not Unix time; its unit is implementation-dependent. Use it for elapsed-time calculations only. |

## `ura.fn`

`ura.fn` contains general Lua helpers. Functions whose names start with `_` are runtime internals and are not part of the public reference.

| Function | Description |
| --- | --- |
| `validate(value, path, type_name)` | Check that a value is a table and the field at a colon-separated path has the requested Lua type, e.g. `validate(t, "window:rules", "table")`. |
| `split(text, separator)` | Split on a literal string separator; empty leading, interior, and trailing fields are retained. |
| `filter(array, predicate)` | Return array elements for which `predicate(index, value)` is exactly `true`. Iterates with `ipairs`. |
| `find(array, predicate)` | Return the first 1-based index whose value satisfies `predicate(value)`, or `nil`. |
| `for_each(table, callback)` | Call `callback(key, value)` for each pair using `pairs`. |
| `unique(array)` | Return an array with duplicate values removed, preserving the first occurrence. |
| `natural_compare(a, b)` / `natural_sort(array)` | Compare/sort strings in natural order (digit-only values first, then mixed strings, then non-digit strings; digit runs compare numerically). `natural_sort` sorts and returns the input array. |
| `collect_tags(options)` | Return sorted unique tags from windows and, by default, the current output. Pass `{ include_active = false }` to omit current-output tags. |
| `fnmatch(pattern, text, flags)` | Match with libc `fnmatch`; `flags` defaults to `0`. |
| `shell(command)` | Run a shell command and return all stdout. Treat the command as trusted input. |
| `load(path)` / `load_dir(path)` | Add `path`'s Lua patterns or patterns under its immediate entries to `package.path`; they do not execute a Lua module. |
| `exists(path)` | Return whether `path` can be opened for reading. |
| `find_config_path()` | Return the selected config file path or `nil`. See [configuration loading](../lua-api.md#configuration-loading). |
| `copy(value)` | Deep-copy tables, including keys and metatables; handles cyclic references. Non-tables are returned unchanged. |
| `lshift(value, count)` / `rshift(value, count)` | Left/right shift using an FFI `uint64_t`; results are LuaJIT FFI integer values. |

`ura.fn.shell`, `ura.api.spawn`, and the FFI helpers can execute or access low-level operations. Use them only with trusted configuration and plugin code.
