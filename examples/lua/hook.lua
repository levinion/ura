ura.hook.set("prepare", function()
  ura.fn.set_env("WLR_RENDERER", "vulkan")
  ura.fn.set_env("WLR_NO_HARDWARE_CURSORS", "0")
  ura.fn.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.fn.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)

ura.hook.set("ready", function()
  os.execute("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2 &")
  ura.fn.set_env("DISPLAY", ":0")
  os.execute("xwayland-satellite &")
  os.execute("swaybg -i ~/.config/ura/assets/bg.jpg &")
  os.execute("waybar &")
  os.execute("mako &")
  os.execute("otd-daemon &")
  os.execute("fcitx5 -rd &")
  os.execute("wl-clip-persist -c both --reconnect-tries 3 &")
  os.execute(
    [[swayidle -w timeout 10 'uracil -c "ura.input.cursor.set_visible(false)"' resume 'uracil -c "ura.input.cursor.set_visible(true)"' &]])
  ura.input.keyboard.set_repeat(40, 300)
  ura.input.cursor.set_theme("", 18)
end)

ura.hook.set("reload", function()
  ura.input.keyboard.set_repeat(40, 300)
  ura.input.cursor.set_theme("", 18)
end)

ura.hook.set("focus-change", function()
  os.execute("pkill -SIGRTMIN+9 waybar &")
end)

ura.hook.set("workspace-change", function()
  os.execute("pkill -SIGRTMIN+8 waybar &")
end)

ura.hook.set("window-new", function(index)
  local win = ura.win.get(index)
  if not win then return end
  if string.match(win.app_id, "fzfmenu") then
    ura.win.set_layout(win.index, "floating")
    ura.win.resize(win.index, 1000, 600)
    ura.win.center(win.index)
  end
  if string.match(win.app_id, "scrcpy") then
    ura.win.set_layout(win.index, "floating")
    ura.win.resize(win.index, 640, 360)
    ura.win.center(win.index)
  end
end)
