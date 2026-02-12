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
  local tags = ura.fn.collect_tags()
  local active_tag = ura.class.UraOutput:current():tags()[1]
  local index = ura.fn.find(tags, active_tag)
  if index - 1 >= 1 then
    ura.class.UraOutput:current():set_tags({ tags[index - 1] })
  end
end)

ura.keymap.set("ctrl+right", function()
  local tags = ura.fn.collect_tags()
  local active_tag = ura.class.UraOutput:current():tags()[1]
  local index = ura.fn.find(tags, active_tag)
  if index + 1 <= #tags then
    ura.class.UraOutput:current():set_tags({ tags[index + 1] })
  end
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

for i = 1, 9 do
  ura.keymap.set("super+" .. tostring(i), function()
    ura.class.UraOutput:current():set_tags({ tostring(i) })
  end)
end

ura.keymap.set("super+0", function()
  ura.class.UraOutput:current():set_tags({ "10" })
end)

ura.hook.add("window-new", function(e)
  ura.class.UraWindow:new(e.id):focus()
end)
