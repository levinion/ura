ura.hook.set("prepare", function()
  ura.fn.set_env("XDG_CURRENT_DESKTOP", "ura")
  ura.fn.set_env("XDG_SESSION_TYPE", "wayland")
  ura.fn.set_env("WLR_RENDERER", "vulkan")
  ura.fn.set_env("WLR_NO_HARDWARE_CURSORS", "0")
  ura.fn.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.fn.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)

ura.hook.set("ready", function()
  ura.fn.set_env("DISPLAY", ":0")
  ura.api.spawn("xwayland-satellite")
  ura.api.spawn("swaybg -i ~/.config/ura/assets/bg.jpg")
  ura.api.spawn("waybar")
  ura.api.spawn("mako")
  ura.api.spawn("fcitx5 -rd")
  ura.api.spawn("wl-clip-persist -c both --reconnect-tries 3")
  ura.api.spawn(
    [[swayidle -w timeout 10 'uracil ~/.config/ura/scripts/hide_cursor.lua' resume 'uracil ~/.config/ura/scripts/hide_cursor.lua -t']])
  ura.input.keyboard.set_repeat(40, 300)
  ura.input.cursor.set_theme({ size = 18 })
end)

ura.hook.set("reload", function()
  ura.input.keyboard.set_repeat(40, 300)
  ura.input.cursor.set_theme({ size = 18 })
end)

ura.hook.set("focus-change", function()
  ura.api.spawn("pkill -SIGRTMIN+9 waybar")
end)

ura.hook.set("workspace-change", function()
  ura.api.spawn("pkill -SIGRTMIN+8 waybar")
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
