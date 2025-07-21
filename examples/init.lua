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
  os.execute("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2 &")
  ura.set_keyboard_repeat(40, 300)
  ura.cursor_theme("", 18)
  ura.tiling_gap { outer_t = 0 };
end

ura.hook("startup", function()
  os.execute("swaybg -i ~/.config/i3/assets/bg.jpg &")
  os.execute("waybar &")
  os.execute("openrgb --startminimized -p default &")
  os.execute("mygo -p 4611 $HOME/.config/i3/assets/catppuccin-homepage &")
  everytime()
end)

ura.hook("reload", function()
  everytime()
end)
