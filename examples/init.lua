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

ura.set_output_scale(1)
ura.set_keyboard_repeat(40, 300)
