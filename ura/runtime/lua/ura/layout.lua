local M = {}

M.LAYOUTS = {}

---@param name string
---@param opt table
function M.register(name, opt)
  M.LAYOUTS[name] = opt
end

---@param win UraWindow
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
  win:update_userdata({ layout = layout })
  M.enter(win)
  M.apply(win)
end

---@param win UraWindow
---@return string|nil
function M.get(win)
  local userdata = win:userdata()
  if ura.fn.validate(userdata, "layout", "string") then
    return userdata.layout
  end
  return nil
end

---@param win UraWindow
function M.enter(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":enter", "function") then
    M.LAYOUTS[layout]["enter"](win)
  end
end

---@param win UraWindow
function M.apply(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":apply", "function") then
    M.LAYOUTS[layout]["apply"](win)
  end
end

---@param win UraWindow
function M.leave(win)
  local layout = M.get(win)
  assert(layout)
  if ura.fn.validate(M.LAYOUTS, layout .. ":leave", "function") then
    M.LAYOUTS[layout]["leave"](win)
  end
end

return M
