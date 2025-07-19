local ura = require("ura")

ura.env("LIBVA_DRIVER_NAME", "nvidia");
ura.env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
ura.env("ELECTRON_OZONE_PLATFORM_HINT", "auto")
ura.env("XCURSOR_SIZE", "24")

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
  os.execute("rmpc togglepause")
end)

local function everytime()
  ura.set_output_scale(2)
  ura.set_keyboard_repeat(40, 300)
  ura.cursor_theme("", 18)
  ura.tiling_gap(10, 10);
  ura.set_output_refresh(120)
end

ura.hook("startup", function()
  os.execute("waybar &")
  everytime()
end)

ura.hook("reload", function()
  everytime()
end)
