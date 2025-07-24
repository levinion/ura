-- This configuration file is provided as an example only. It may not work on your system.
-- Please make sure you understand each part before using it. Do not run any code you're unfamiliar with.
-- Have a great day!

local ura = require("ura")

local function everytime()
  os.execute("wlr-randr --output DP-5 --mode 3840x2160@159.977005Hz --scale 2 &")
  ura.set_keyboard_repeat(40, 300)
  ura.set_cursor_theme("", 18)
  ura.tiling_gap { outer_t = 0 };
end

ura.hook("prepare", function()
  require("env")
  require("keymap")
end)

ura.hook("ready", function()
  ura.env("DISPLAY", ":10")
  os.execute("xwayland-satellite &")
  os.execute("swaybg -i ~/.config/i3/assets/bg.jpg &")
  os.execute("waybar &")
  os.execute("openrgb --startminimized -p default &")
  os.execute("sudo sing-box run -c $HOME/.config/sing-box/config.yaml -D $HOME/.config/sing-box &")
  os.execute("mygo -p 4611 $HOME/.config/i3/assets/catppuccin-homepage &")
  everytime()
end)

ura.hook("reload", function()
  everytime()
end)
