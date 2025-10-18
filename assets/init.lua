ura.keymap.set("super+t", function()
  ura.api.spawn("foot")
end)

ura.keymap.set("super+q", function()
  ura.cmd.close()
end)

ura.keymap.set("alt+space", function()
  ura.cmd.toggle_layout("floating")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("super+shift+r", function()
  ura.api.reload()
end)

ura.keymap.set("super+f", function()
  ura.cmd.toggle_layout("fullscreen")
end)

ura.keymap.set("ctrl+left", function()
  ura.cmd.switch_prev()
end)

ura.keymap.set("ctrl+right", function()
  ura.cmd.switch_next()
end)

ura.keymap.set("ctrl+shift+left", function()
  ura.cmd.move_to_prev()
end)

ura.keymap.set("ctrl+shift+right", function()
  ura.cmd.move_to_next()
end)

ura.keymap.set("super+h", function()
  ura.cmd.focus_prev()
end)

ura.keymap.set("super+l", function()
  ura.cmd.focus_next()
end)

ura.keymap.set("super+shift+h", function()
  ura.cmd.swap_prev()
end)

ura.keymap.set("super+shift+l", function()
  ura.cmd.swap_next()
end)

ura.api.set_hook("prepare", function() end)

ura.api.set_hook("ready", function() end)

ura.api.set_hook("reload", function() end)
