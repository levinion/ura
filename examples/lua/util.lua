local M = {}

---@param w UraWindow
---@param layout string
function M.toggle_layout(w, layout)
  ura.win.set_layout(w.index, w.layout ~= layout and layout or (w.last_layout or "tiling"))
end

return M
