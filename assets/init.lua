require("builtin.layout.tiling").setup()
require("builtin.layout.fullscreen").setup()
require("builtin.layout.floating").setup()

ura.keymap.set("super+t", function()
  ura.api.spawn("foot")
end)

ura.keymap.set("super+q", function()
  ura.class.UraWindow:current():close()
end)

ura.keymap.set("alt+space", function()
  ura.class.UraWindow:current():toggle_layout("floating")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("super+shift+r", function()
  ura.api.reload()
end)

ura.keymap.set("super+f", function()
  ura.class.UraWindow:current():toggle_layout("fullscreen")
end)

ura.keymap.set("ctrl+left", function()
  local tag = tonumber(ura.class.UraOutput:current():tags()[1]) - 1
  if tag < 0 then
    return
  end
  ura.class.UraOutput:current():set_tags({ tostring(tag) })
end)

ura.keymap.set("ctrl+right", function()
  local tag = tonumber(ura.class.UraOutput:current():tags()[1]) + 1
  ura.class.UraOutput:current():set_tags({ tostring(tag) })
end)

ura.keymap.set("super+h", function()
  ura.cmd.focus_left()
end)

ura.keymap.set("super+l", function()
  ura.cmd.focus_right()
end)

ura.keymap.set("super+j", function()
  ura.cmd.focus_down()
end)

ura.keymap.set("super+k", function()
  ura.cmd.focus_up()
end)

ura.api.set_hook("prepare", function() end)

ura.api.set_hook("ready", function() end)

ura.api.set_hook("reload", function() end)
