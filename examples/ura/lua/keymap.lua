local ura = require("ura")

ura.map("super", "t", function()
  os.execute("alacritty &")
end)

ura.map("super", "w", function()
  os.execute("firefox-developer-edition &")
end)

ura.map("super", "e", function()
  ura.terminate()
end)

ura.map("super", "q", function()
  ura.close_window()
end)

ura.map("super", "r", function()
  ura.reload()
end)

ura.map("super", "f", function()
  ura.fullscreen()
end)

ura.map("ctrl", "left", function()
  local index = ura.current_workspace()
  ura.switch_workspace(index - 1)
end)

ura.map("ctrl", "right", function()
  local index = ura.current_workspace()
  ura.switch_workspace(index + 1)
end)

ura.map("ctrl+shift", "left", function()
  local index = ura.current_workspace()
  ura.move_to_workspace(index - 1)
end)

ura.map("ctrl+shift", "right", function()
  local index = ura.current_workspace()
  ura.move_to_workspace(index + 1)
end)

ura.map("super", "h", function()
  local index = ura.current_toplevel()
  ura.focus(index - 1)
end)

ura.map("super", "l", function()
  local index = ura.current_toplevel()
  ura.focus(index + 1)
end)

ura.map("super", "p", function()
  os.execute("rmpc togglepause &")
end)

ura.map("alt", "a", function()
  os.execute('grim -g "$(slurp)" - | wl-copy &')
end)
