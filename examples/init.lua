-- This configuration file is provided as an example only. It may not work on your system.
-- Please make sure you understand each part before using it. Do not run any code you're unfamiliar with.
-- Have a great day!
-- Copyright (c) 2025 levinion

local function set_keymap()
  ura.keymap.set("super+t", function()
    os.execute("alacritty -e tmux &")
  end)
  ura.keymap.set("super+w", function()
    os.execute("firefox-developer-edition &")
  end)
  ura.keymap.set("super+e", function()
    os.execute("alacritty -e yazi &")
  end)
  ura.keymap.set("super+q", function()
    local w = ura.win.get_current()
    if w then ura.win.close(w.index) end
  end)
  ura.keymap.set("super+space", function()
    os.execute("fzfmenu &")
  end)
  ura.keymap.set("alt+space", function()
    local w = ura.win.get_current()
    if w then ura.win.set_floating(w.index, not w.floating) end
  end)
  ura.keymap.set("super+shift+e", function()
    ura.api.terminate()
  end)
  ura.keymap.set("super+shift+r", function()
    ura.api.reload()
  end)
  ura.keymap.set("super+f", function()
    local w = ura.win.get_current()
    if w then ura.win.set_fullscreen(w.index, not w.fullscreen) end
  end)
  ura.keymap.set("ctrl+left", function()
    local index = ura.ws.get_current().index
    ura.ws.switch(index - 1)
    ura.ws.destroy(index)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl+right", function()
    local index = ura.ws.get_current().index
    ura.ws.switch(index + 1)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl+shift+left", function()
    local ws = ura.ws.get_current()
    local win = ura.win.get_current()
    if not win then return end
    ura.win.move_to_workspace(win.index, ws.index - 1)
    ura.ws.destroy(ws.index)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl+shift+right", function()
    local ws = ura.ws.get_current()
    local win = ura.win.get_current()
    if not win then return end
    ura.win.move_to_workspace(win.index, ws.index + 1)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("super+h", function()
    local index = ura.win.get_current().index
    ura.win.focus(index - 1)
  end)
  ura.keymap.set("super+l", function()
    local index = ura.win.get_current().index
    ura.win.focus(index + 1)
  end)
  ura.keymap.set("super+p", function()
    os.execute("rmpc togglepause &")
  end)
  ura.keymap.set("alt+a", function()
    os.execute('grim -g "$(slurp)" - | wl-copy &')
  end)
  ura.keymap.set("super+shift+m", function()
    local index = ura.win.get_current().index
    ura.win.move_to_workspace(index, -1)
  end)
end


local function everytime()
  set_keymap()
  os.execute("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2 &")
  -- os.execute("wlr-randr --output DP-5 --mode 3840x2160@159.977005Hz --scale 2 &")
  ura.input.keyboard.set_repeat(40, 300)
  ura.input.cursor.set_theme("", 18)
  ura.layout.tilling.gap.outer.top = 0
end

ura.hook.set("prepare", function()
  ura.fn.set_env("WLR_RENDERER", "gles2")
  ura.fn.set_env("WLR_NO_HARDWARE_CURSORS", "1")
  ura.fn.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.fn.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)

ura.hook.set("ready", function()
  ura.fn.set_env("DISPLAY", ":0")
  os.execute("xwayland-satellite &")
  os.execute("swaybg -i ~/.config/i3/assets/bg.jpg &")
  os.execute("waybar &")
  os.execute("openrgb --startminimized -p default &")
  os.execute("sudo sing-box run -c $HOME/.config/sing-box/config.yaml -D $HOME/.config/sing-box &")
  os.execute("mygo -p 4611 $HOME/.config/i3/assets/catppuccin-homepage &")
  os.execute("dunst &")
  os.execute("otd-daemon &")
  os.execute("fcitx5 -rd &")
  everytime()
end)

ura.hook.set("reload", function()
  everytime()
end)

ura.hook.set("activate", function()
  os.execute("pkill -SIGRTMIN+8 waybar &")
end)
