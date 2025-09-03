local M = {}

---@param w UraWindow
---@param layout string
function M.toggle_layout(w, layout)
  ura.win.set_layout(w.index, w.layout ~= layout and layout or (w.last_layout or "tiling"))
end

---@param rules table
function M.window_rules(rules)
  ura.hook.set("window-new", function(index)
    local win = ura.win.get(index)
    if not win then return end
    for _, rule in ipairs(rules) do
      local match = true

      if rule.app_id then
        if not string.match(win.app_id, rule.app_id) then
          match = false
        end
      end

      if rule.title then
        if not (win.title and string.match(win.title, rule.title)) then
          match = false
        end
      end

      if match then
        if rule.layout then
          ura.win.set_layout(win.index, rule.layout)
        end
        if rule.width and rule.height then
          ura.win.resize(win.index, rule.width, rule.height)
        end
        if rule.center then
          ura.win.center(win.index)
        end
        break
      end
    end
  end)
end

return M
