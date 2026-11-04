require("builtin.layout").setup()

require("builtin.animation").setup()

ura.animation.define("window-move", {
  duration = 200,
  easing = "out_cubic",
})

ura.animation.define("window-resize", {
  duration = 200,
  easing = "out_cubic",
})

ura.keymap.set({ "super+t" }, function()
  ura.api.spawn("foot")
end)

ura.keymap.set({ "super+q" }, function()
  ura.class.UraWindow:current():close()
end)

ura.keymap.set({ "alt+space" }, function()
  ura.class.UraWindow:current():toggle_layout("floating")
end)

ura.keymap.set({ "super+shift+e" }, function()
  ura.api.terminate()
end)

ura.keymap.set({ "super+shift+r" }, function()
  ura.cmd.reload()
end)

ura.keymap.set({ "super+f" }, function()
  ura.class.UraWindow:current():toggle_layout("fullscreen")
end)

ura.keymap.set({ "super+shift+f" }, function()
  ura.class.UraWindow:current():toggle_layout("maximize")
end)

ura.keymap.set({ "ctrl+left", "super+wheelup" }, function()
  ura.api.shift_tag_index(-1)
end)

ura.keymap.set({ "ctrl+right", "super+wheeldown" }, function()
  ura.api.shift_tag_index(1)
end)

ura.keymap.set({ "ctrl+shift+left", "super+shift+wheelup" }, function()
  ura.api.shift_window_tag_index(-1, true)
end)

ura.keymap.set({ "ctrl+shift+right", "super+shift+wheeldown" }, function()
  ura.api.shift_window_tag_index(1, true)
end)

ura.keymap.set({ "ctrl+alt+left", "super+mouseside" }, function()
  ura.api.switch_tag_namespace(-1)
end)

ura.keymap.set({ "ctrl+alt+right", "super+mouseextra" }, function()
  ura.api.switch_tag_namespace(1)
end)

ura.keymap.set({ "super+h" }, function()
  ura.cmd.focus_left()
end)

ura.keymap.set({ "super+l" }, function()
  ura.cmd.focus_right()
end)

ura.keymap.set({ "super+j" }, function()
  ura.cmd.focus_down()
end)

ura.keymap.set({ "super+k" }, function()
  ura.cmd.focus_up()
end)

ura.keymap.set({ "super+left" }, function()
  ura.class.UraWindow:current():shrink(0.1)
end)

ura.keymap.set({ "super+right" }, function()
  ura.class.UraWindow:current():expand(0.1)
end)

for i = 1, 10 do
  local key = i == 10 and "0" or tostring(i)
  ura.keymap.set({ "super+" .. key }, function()
    ura.api.set_tag_index(i)
  end)
end

ura.hook.add("window-new", function(e)
  local win = ura.class.UraWindow:new(e.id)

  if win:layout() == nil then
    win:set_layout("tiling")
  end
end)

ura.hook.add("window-map", function(e)
  local win = ura.class.UraWindow:new(e.id)
  if e.initial == true then
    win:focus()
  end
end)

ura.hook.add("window-focus", function(e)
  local win = ura.class.UraWindow:new(e.id)
  win:set_border_color("#89b4fa")
  win:set_opacity(1.)
end)

ura.hook.add("window-unfocus", function(e)
  local win = ura.class.UraWindow:new(e.id)
  win:set_border_color("#00000000")
  win:set_opacity(0.9)
end)
