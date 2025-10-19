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

M.register("fullscreen", {
  enter = function(win)
    ura.api.set_window_draggable(win, false)
    ura.api.set_window_z_index(win, 250)
    ura.api.set_window_fullscreen(win, true)
  end,
  apply = function(win)
    local output = ura.api.get_window_output(win)
    assert(output)
    local geo = ura.api.get_output_logical_geometry(output)
    assert(geo)
    ura.api.resize_window(win, geo.width, geo.height)
    ura.api.move_window(win, geo.x, geo.y)
  end,
  leave = function(win)
    ura.api.set_window_fullscreen(win, false)
  end,
})

ura.hook.set("output-usable-geometry-change", function(_)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.fullscreen", priority = 40, desc = "re-apply layout as usable geometry change" })

M.register("floating", {
  enter = function(win)
    ura.api.set_window_draggable(win, true)
    ura.api.set_window_z_index(win, 150)
    -- recover window size
    local userdata = ura.api.get_userdata(win)
    assert(userdata)
    assert(ura.fn.validate(userdata, "floating", "table"))
    if userdata.floating.width and userdata.floating.height then
      ura.api.resize_window(win, userdata.floating.width, userdata.floating.height)
    end
    if userdata.floating.x and userdata.floating.y then
      ura.api.move_window(win, userdata.floating.x, userdata.floating.y)
    end
  end,
  leave = function(win)
    local geo = ura.api.get_window_geometry(win)
    assert(geo)
    ura.fn.update_userdata(win, { floating = geo })
  end,
})

ura.hook.set("window-new", function(e)
  local win = e.id
  assert(win)
  local geo = ura.api.get_window_geometry(win)
  assert(geo)
  ura.fn.update_userdata(win, { floating = geo })
end, {
  ns = "layout.floating",
  priority = 0,
  desc = "preserve initial geometry",
})

M.register("tiling", {
  enter = function(win)
    ura.api.set_window_draggable(win, false)
    ura.api.set_window_z_index(win, 100)
  end,
  apply = function(win)
    local output = ura.api.get_window_output(win)
    assert(output)
    local usable = ura.api.get_output_usable_geometry(output)
    assert(usable)
    local workspace = ura.api.get_window_workspace(win)
    assert(workspace)
    local windows = ura.api.get_windows(workspace)
    assert(windows)

    local opt = ura.opt["layout.tiling"]
    local outer_r = opt and opt.outer_r or 10
    local outer_l = opt and opt.outer_l or 10
    local outer_t = opt and opt.outer_t or 10
    local outer_b = opt and opt.outer_b or 10
    local inner = opt and opt.inner or 10

    local sum = 0
    local index = 0

    for i = 0, #windows - 1 do
      local w = ura.api.get_window(workspace, i)
      if w then
        if w == win then
          index = sum
        end
        local layout = M.get(w)
        if layout == "tiling" then
          sum = sum + 1
        end
      end
    end

    local gaps = sum - 1
    local w = (usable.width - (outer_r + outer_l) - inner * gaps) / sum
    local h = usable.height - (outer_t + outer_b)
    local x = usable.x + outer_l + (w + inner) * index
    local y = usable.y + outer_t
    ura.api.resize_window(win, w, h)
    ura.api.move_window(win, x, y)
  end,
})

ura.hook.set("window-new", function(e)
  local win = e.id
  assert(win)
  if M.get(win) ~= nil then
    return
  end
  M.set(win, "tiling")
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 100, desc = "set tiling as the fallback layout" })

ura.hook.set("window-resize", function(e)
  assert(M.get(e.id) == "tiling")
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window resized" })

ura.hook.set("window-close", function(_)
  -- we don't know this window's layout since it is destroyed
  -- we have to apply all of the windows
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window closed" })

ura.hook.set("workspace-change", function(_)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as workspace changed" })

ura.hook.set("window-swap", function(_)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window's index is changed" })

ura.hook.set("output-usable-geometry-change", function(_)
  local ws = ura.api.get_current_workspace()
  assert(ws)
  M.apply_workspace(ws)
end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as usable geometry change" })
return M
