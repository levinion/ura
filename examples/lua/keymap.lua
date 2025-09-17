local util = require("util")

ura.keymap.set("super+t", function()
  ura.api.spawn("foot -e tmux")
end)

ura.keymap.set("super+w", function()
  ura.api.spawn("firefox-developer-edition")
end)

ura.keymap.set("super+e", function()
  ura.api.spawn("foot -e yazi")
end)

ura.keymap.set("super+q", function()
  local w = ura.win.get_current()
  if w then ura.win.close(w.index) end
end)

ura.keymap.set("super+space", function()
  ura.api.spawn("fzfmenu")
end)

ura.keymap.set("alt+space", function()
  local w = ura.win.get_current()
  if not w then return end
  util.toggle_layout(w, "floating")
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
  util.toggle_layout(w, "fullscreen")
end)

ura.keymap.set("ctrl+left", function()
  local index = ura.ws.get_current().index
  ura.ws.switch(index - 1)
  ura.ws.destroy(index)
end)

ura.keymap.set("ctrl+right", function()
  local index = ura.ws.get_current().index
  ura.ws.switch_or_create(index + 1)
end)

ura.keymap.set("ctrl+shift+left", function()
  local ws = ura.ws.get_current()
  local win = ura.win.get_current()
  if not win then return end
  ura.win.move_to_workspace(win.index, ws.index - 1)
  ura.ws.switch(ws.index - 1)
  ura.ws.destroy(ws.index)
end)

ura.keymap.set("ctrl+shift+right", function()
  local index = ura.ws.get_current().index
  local win = ura.win.get_current()
  if not win then return end
  ura.win.move_to_workspace_or_create(win.index, index + 1)
  ura.ws.switch(index + 1)
end)

for i = 0, 9 do
  ura.keymap.set("super+" .. i, function()
    ura.ws.switch_or_create(i)
  end)
end

ura.keymap.set("super+h", function()
  local win = ura.win.get_current()
  if not win then return end
  if win.index == 0 and win.workspace_index == 0 then return end
  if win.index >= 1 then
    ura.win.focus(win.index - 1)
  else
    ura.ws.switch(win.workspace_index - 1)
    ura.win.focus(ura.win.size() - 1)
  end
end)

ura.keymap.set("super+l", function()
  local win = ura.win.get_current()
  if not win then return end
  if win.index < ura.win.size() - 1 then
    ura.win.focus(win.index + 1)
  else
    if ura.ws.get_current().index ~= ura.ws.size() - 1 then
      ura.ws.switch(win.workspace_index + 1)
      ura.win.focus(0)
    end
  end
end)

ura.keymap.set("super+shift+h", function()
  local win = ura.win.get_current()
  if not win then return end
  ura.win.swap(win.index, win.index - 1)
end)

ura.keymap.set("super+shift+l", function()
  local win = ura.win.get_current()
  if not win then return end
  ura.win.swap(win.index, win.index + 1)
end)

ura.keymap.set("super+shift+p", function()
  ura.api.spawn('uracil ~/.config/ura/scripts/dpms_off.lua')
end)

ura.keymap.set("super+p", function()
  ura.api.spawn("rmpc togglepause")
end)

ura.keymap.set("alt+a", function()
  ura.api.spawn('grim -g "$(slurp)" - | wl-copy')
end)

ura.keymap.set("super+shift+m", function()
  local index = ura.win.get_current().index
  ura.win.move_to_workspace(index, "scratchpad")
end)

ura.keymap.set("XF86AudioRaiseVolume", function()
  ura.api.spawn("volume -i 5")
end)

ura.keymap.set("XF86AudioLowerVolume", function()
  ura.api.spawn("volume -d 5")
end)

ura.keymap.set("super+shift+s", function()
  ura.api.spawn("swaylock -f -i ~/.config/ura/assets/bg.jpg")
end)

ura.keymap.set("super+up", function()
  local win = ura.win.get_current()
  if not win or not win.layout == "floating" then return end
  ura.win.center(win.index)
end)

ura.keymap.set("super+d", function()
  local w = ura.win.get_current()
  if not w then return end
  util.toggle_layout(w, "always-on-top")
end)
