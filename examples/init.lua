local ura = require("ura")

ura.env("LIBVA_DRIVER_NAME", "nvidia");
ura.env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
ura.env("ELECTRON_OZONE_PLATFORM_HINT", "auto")
ura.env("XCURSOR_SIZE", "24")

ura.map("super", "t", function()
  os.execute("alacritty &")
end)

ura.map("super", "w", function()
  os.execute("firefox &")
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
  ura.prev_workspace()
end)

ura.map("ctrl", "right", function()
  ura.next_workspace()
end)

ura.set_output_scale(2)
ura.set_keyboard_repeat(40, 300)

ura.hook("startup", function()
  os.execute("swaybg -i ~/.config/i3/assets/bg.jpg &")
end)
