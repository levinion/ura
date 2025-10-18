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
  ura.cmd.close()
end)

ura.keymap.set("super+space", function()
  ura.api.spawn("fzfmenu")
end)

ura.keymap.set("alt+space", function()
  ura.cmd.toggle_layout("floating")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("super+shift+r", function()
  ura.api.reload()
end)

ura.keymap.set("super+f", function()
  ura.cmd.toggle_layout("fullscreen")
end)

ura.keymap.set("ctrl+left", function()
  ura.cmd.switch_prev()
end)

ura.keymap.set("ctrl+right", function()
  ura.cmd.switch_next()
end)

ura.keymap.set("ctrl+shift+left", function()
  ura.cmd.move_to_prev()
end)

ura.keymap.set("ctrl+shift+right", function()
  ura.cmd.move_to_next()
end)

ura.keymap.set("super+h", function()
  ura.cmd.focus_prev()
end)

ura.keymap.set("super+l", function()
  ura.cmd.focus_next()
end)

ura.keymap.set("super+shift+h", function()
  ura.cmd.swap_prev()
end)

ura.keymap.set("super+shift+l", function()
  ura.cmd.swap_next()
end)

ura.keymap.set("super+shift+p", function()
  ura.api.spawn("uracil ~/.config/ura/scripts/dpms_off.lua")
end)

ura.keymap.set("super+p", function()
  ura.api.spawn("rmpc togglepause")
end)

ura.keymap.set("alt+a", function()
  ura.api.spawn(
    [[grim -g "$(slurp)" - | satty --filename - --fullscreen --output-filename ~/Pictures/Catch/$(date +%Y-%m-%d-%H-%M-%S).png]]
  )
end)

ura.keymap.set("super+shift+m", function()
  local win = ura.api.get_current_window()
  local scratchpad = ura.api.get_named_workspace("scratchpad")
  if scratchpad == nil then
    ura.api.create_named_workspace("scratchpad")
    scratchpad = ura.api.get_named_workspace("scratchpad")
  end
  if win and scratchpad then
    ura.api.move_window_to_workspace(win, scratchpad)
  end
end)

ura.keymap.set("XF86AudioRaiseVolume", function()
  ura.api.spawn("volume -i 5")
end)

ura.keymap.set("XF86AudioLowerVolume", function()
  ura.api.spawn("volume -d 5")
end)

ura.keymap.set("XF86MonBrightnessUp", function()
  ura.api.spawn("brightness -i 5")
end)

ura.keymap.set("XF86MonBrightnessDown", function()
  ura.api.spawn("brightness -d 5")
end)

ura.keymap.set("super+shift+s", function()
  ura.api.spawn("swaylock -f -i ~/.config/ura/assets/bg.jpg")
end)
