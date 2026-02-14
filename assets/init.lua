require("builtin.layout.tiling").setup()
require("builtin.layout.fullscreen").setup()
require("builtin.layout.floating").setup()

ura.keymap.set("super+t", function()
  ura.api.spawn("foot")
end)

ura.keymap.set("super+q", function()
  ura.class.UraWindow:current():close()
end)

ura.keymap.set("alt+space", function()
  ura.class.UraWindow:current():toggle_layout("floating")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("super+f", function()
  ura.class.UraWindow:current():toggle_layout("fullscreen")
end)

ura.keymap.set("ctrl+left", function()
  local seg = ura.class.UraSegment:from_tag(ura.class.UraOutput:current():tags()[1])
  assert(seg)
  if seg.index > 1 then
    seg.index = seg.index - 1
    ura.class.UraOutput:current():set_tags({ seg:tag() })
  end
end)

ura.keymap.set("ctrl+right", function()
  local seg = ura.class.UraSegment:from_tag(ura.class.UraOutput:current():tags()[1])
  assert(seg)
  seg.index = seg.index + 1
  ura.class.UraOutput:current():set_tags({ seg:tag() })
end)

ura.keymap.set("ctrl+alt+left", function()
  local seg = ura.class.UraSegment:from_tag(ura.class.UraOutput:current():tags()[1])
  assert(seg)
  local segs = ura.class.UraSegment:all()
  local index = ura.fn.find(segs, seg)
  if index > 1 then
    ura.class.UraOutput:current():set_tags({ segs[index - 1]:tag() })
  end
end)

ura.keymap.set("ctrl+alt+right", function()
  local seg = ura.class.UraSegment:from_tag(ura.class.UraOutput:current():tags()[1])
  assert(seg)
  local segs = ura.class.UraSegment:all()
  local index = ura.fn.find(segs, seg)
  if index < #segs then
    ura.class.UraOutput:current():set_tags({ segs[index + 1]:tag() })
  end
end)

ura.keymap.set("super+h", function()
  ura.cmd.focus_left()
end)

ura.keymap.set("super+l", function()
  ura.cmd.focus_right()
end)

ura.keymap.set("super+j", function()
  ura.cmd.focus_down()
end)

ura.keymap.set("super+k", function()
  ura.cmd.focus_up()
end)

for i = 1, 10 do
  local key = i == 10 and "0" or tostring(i)
  ura.keymap.set("super+" .. key, function()
    local output = ura.class.UraOutput:current()
    assert(output)
    local seg = ura.class.UraSegment:from_tag(output:tags()[1])
    assert(seg)
    seg.index = i
    output:set_tags({ seg:tag() })
  end)
end

ura.hook.add("window-new", function(e)
  local win = ura.class.UraWindow:new(e.id)
  win:set_layout("tiling")
  win:focus()
end)
