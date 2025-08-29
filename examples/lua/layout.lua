ura.layout.set("always-on-top", function(index)
  local win = ura.win.get(index)
  if not win then return nil end
  if win.first_commit_after_layout_change then
    ura.win.set_z_index(win.index, 500)
    ura.win.set_draggable(win.index, true)
  end
  return nil
end)
