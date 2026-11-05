# Idle timeouts

Idle stages are named C++ timers. Each stage can optionally carry its own idle
and resume callbacks; `on_idle` and `on_resume` are both optional.

```lua
ura.api.add_idle_timeout(
  "cursor",
  10,
  function(event)
    ura.api.set_cursor_visible(false)
  end,
  function(event)
    ura.api.set_cursor_visible(true)
  end
)
```

Multiple stages are cumulative and reset on input activity:

```lua
ura.api.add_idle_timeout("cursor", 5, function()
  ura.api.set_cursor_visible(false)
end, function()
  ura.api.set_cursor_visible(true)
end)

ura.api.add_idle_timeout("dpms", 10, function()
  for _, output in ipairs(ura.class.UraOutput:all()) do
    output:set_dpms(false)
  end
end, function()
  for _, output in ipairs(ura.class.UraOutput:all()) do
    output:set_dpms(true)
  end
end)
```

A stage-local `on_resume` runs only if that stage fired. The global `resume`
hook then runs once for shared cleanup. If `on_idle` or `on_resume` is omitted,
the stage still fires the corresponding global `idle`/`resume` hook.

## Functions

| Function | Description |
| --- | --- |
| `ura.api.add_idle_timeout(name, seconds, on_idle?, on_resume?)` | Add or replace a named idle stage. |
| `ura.api.remove_idle_timeout(name)` | Remove a stage. |
| `ura.api.clear_idle_timeouts()` | Remove every idle stage. |
| `ura.api.is_idle()` | Whether at least one stage fired and activity has not resumed. |
| `ura.api.set_idle_inhibitor(enabled)` | Inhibit the idle subsystem. |

## Global hooks

Global hooks remain available for plugins that do not want per-stage
callbacks:

```lua
ura.hook.add("idle", function(event)
  -- event.name, event.timeout
end)

ura.hook.add("resume", function(event)
  -- event.idle: seconds since the first fired stage
end)
```

## DPMS

DPMS is exposed per output:

```lua
ura.class.UraOutput:current():set_dpms(false)
```

Use `ura.class.UraOutput:all()` to apply a policy to every output. Disabling an
output through DPMS leaves its layout and windows intact. Like niri, input
activity wakes DPMS-off outputs before idle `resume` callbacks run. This is
intentional for lock screens and manual `set_dpms(false)` calls, where no idle
stage may have fired.

A timeout can still pair its own policy callbacks:

```lua
ura.api.add_idle_timeout("dpms", 600, function()
  for _, output in ipairs(ura.class.UraOutput:all()) do
    output:set_dpms(false)
  end
end, function()
  for _, output in ipairs(ura.class.UraOutput:all()) do
    output:set_dpms(true)
  end
end)
```
