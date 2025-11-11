local M = {}

M.LAYOUTS = {}

---@param name string
---@param opt table
function M.register(name, opt)
  M.LAYOUTS[name] = opt
end

---@param win integer
---@param layout string
function M.set(win, layout)
  assert(type(layout) == "string")
  local old = M.get(win)
  if old == layout then
    return
  end
  if old then
    M.leave(win)
  end
  ura.fn.update_userdata(win, { layout = layout })
  M.enter(win)
  M.apply(win)
end

---@param win integer
---@return string|nil
function M.get(win)
  local userdata = ura.api.get_userdata(win)
  if ura.fn.validate(userdata, "layout", "string") then
    return userdata.layout
  end
  return nil
end

function M.enter(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":enter", "function") then
    M.LAYOUTS[layout]["enter"](win)
  end
end

---@param win integer
function M.apply(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":apply", "function") then
    M.LAYOUTS[layout]["apply"](win)
  end
end

---@param win integer
function M.leave(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":leave", "function") then
    M.LAYOUTS[layout]["leave"](win)
  end
end

---@param ws integer
function M.apply_workspace(ws)
  local windows = ura.api.get_windows(ws)
  assert(windows)
  for index = 0, #windows - 1 do
    local w = ura.api.get_window(ws, index)
    if w then
      M.apply(w)
    end
  end
end

return M
