ura.keymap.set("super+t", function()
  os.execute("alacritty &")
end)

ura.keymap.set("super+q", function()
  local w = ura.win.get_current()
  if w then ura.win.close(w.index) end
end)

ura.keymap.set("alt+space", function()
  local w = ura.win.get_current()
  if not w then return end
  ura.win.set_layout(w.index, w.layout ~= "floating" and "floating" or "tiling")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("super+shift+r", function()
  ura.api.reload()
end)

ura.keymap.set("super+f", function()
  local w = ura.win.get_current()
  if not w then return end
  ura.win.set_layout(w.index, w.layout ~= "fullscreen" and "fullscreen" or "tiling")
end)

ura.keymap.set("ctrl+left", function()
  local index = ura.ws.get_current().index
  ura.ws.switch(index - 1)
  ura.ws.destroy(index)
end)

ura.keymap.set("ctrl+right", function()
  local index = ura.ws.get_current().index
  ura.ws.switch(index + 1)
end)

ura.keymap.set("ctrl+shift+left", function()
  local ws = ura.ws.get_current()
  if ws.index == 0 then return end
  local win = ura.win.get_current()
  if not win then return end
  ura.win.move_to_workspace(win.index, ws.index - 1)
  ura.ws.destroy(ws.index)
end)

ura.keymap.set("ctrl+shift+right", function()
  local ws = ura.ws.get_current()
  local win = ura.win.get_current()
  if not win then return end
  ura.win.move_to_workspace(win.index, ws.index + 1)
end)

ura.keymap.set("super+h", function()
  local index = ura.win.get_current().index
  ura.win.focus(index - 1)
end)

ura.keymap.set("super+l", function()
  local index = ura.win.get_current().index
  ura.win.focus(index + 1)
end)

ura.keymap.set("super+shift+m", function()
  local index = ura.win.get_current().index
  ura.win.move_to_workspace(index, -1)
end)

ura.hook.set("prepare", function() end)

ura.hook.set("ready", function() end)

ura.hook.set("focus-change", function() end)

ura.hook.set("workspace-change", function() end)

ura.hook.set("window-new", function() end)
