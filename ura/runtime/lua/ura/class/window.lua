--- @class UraWindow
--- @field id integer
local UraWindow = {}
UraWindow.__index = UraWindow

function UraWindow.__eq(a, b)
  if rawequal(a, b) then
    return true
  end
  if type(a) == "table" and type(b) == "table" then
    return a.id == b.id
  end
  return false
end

--- @param id integer
--- @return UraWindow
function UraWindow:new(id)
  local instance = setmetatable({}, UraWindow)
  instance.id = id
  return instance
end

--- @return UraWindow|nil
function UraWindow:current()
  local id = ura.api.get_current_window()
  return id and UraWindow:new(id) or nil
end

--- @return table<UraWindow>
function UraWindow:all()
  local wins = ura.api.get_all_windows()
  local t = {}
  for _, win in ipairs(wins) do
    table.insert(t, ura.class.UraWindow:new(win))
  end
  return t
end

---@param tags table<string>
---@return table<UraWindow>
function UraWindow:from_tags(tags)
  local wins = ura.api.get_all_windows() or {}
  local t = {}
  for _, win in ipairs(wins) do
    local w = ura.class.UraWindow:new(win)
    local contains = false
    for _, tag in ipairs(tags) do
      if ura.fn.find(w:tags(), function(v)
        return v == tag
      end) then
        contains = true
        break
      end
    end
    if contains then
      table.insert(t, w)
    end
  end
  return t
end

---@param x integer
---@param y integer
---@return table<UraWindow>
function UraWindow:from_pos(x, y)
  local wins = self:all()
  local t = {}
  for _, win in ipairs(wins) do
    local geo = win:geometry()
    if win:is_mapped() and geo.x <= x and geo.x + geo.width >= x and geo.y <= y and geo.y + geo.height >= y then
      table.insert(t, win)
    end
  end
  return t
end

function UraWindow:close()
  ura.api.close_window(self.id)
end

function UraWindow:focus()
  ura.api.focus_window(self.id)
end

function UraWindow:activate()
  ura.api.activate_window(self.id)
end

---@return UraOutput|nil
function UraWindow:output()
  local id = ura.api.get_window_output(self.id)
  return id and ura.class.UraOutput:new(id) or nil
end

---@return string|nil
function UraWindow:app_id()
  return ura.api.get_window_app_id(self.id)
end

---@return string|nil
function UraWindow:title()
  return ura.api.get_window_title(self.id)
end

---@return table|nil
function UraWindow:geometry()
  local userdata = self:userdata()
  if userdata and userdata.geometry then
    return userdata.geometry
  else
    self:update_userdata(function(t)
      t.geometry = ura.api.get_window_geometry(self.id)
    end)
    return self:userdata().geometry
  end
end

---@return integer|nil
function UraWindow:z_index()
  return ura.api.get_window_z_index(self.id)
end

--- @param z integer
function UraWindow:set_z_index(z)
  ura.api.set_window_z_index(self.id, z)
end

---@return boolean|nil
function UraWindow:is_fullscreen()
  return ura.api.is_window_fullscreen(self.id)
end

--- @param flag boolean
function UraWindow:set_fullscreen(flag)
  ura.api.set_window_fullscreen(self.id, flag)
end

---@return boolean|nil
function UraWindow:is_mapped()
  return ura.api.is_window_mapped(self.id)
end

---@return boolean|nil
function UraWindow:is_focused()
  return ura.api.is_window_focused(self.id)
end

--- @param x integer
--- @param y integer
--- @param opt { duration?: integer, fps?: number }|nil
function UraWindow:move(x, y, opt)
  if self:userdata().move_timer then
    ura.api.clear_interval(self:userdata().move_timer)
  end

  local geo = self:geometry()
  assert(geo)
  local start_x = geo.x
  local start_y = geo.y

  self:update_userdata(function(t)
    t.geometry = t.geometry or {}
    t.geometry.x = x
    t.geometry.y = y
  end)

  local duration = opt and opt.duration or ura.opt.animation_duration or 500
  local fps = opt and opt.fps or ura.opt.animation_fps or 60

  if duration <= 0 then
    ura.api.move_window(self.id, x, y)
    return
  end

  self:update_userdata(function(tb)
    local start_time = ura.api.time_since_epoch() / 1e6

    tb.move_timer = ura.api.set_interval(function()
      local now = ura.api.time_since_epoch() / 1e6
      local elapsed = now - start_time
      local t = math.min(elapsed / duration, 1.0)

      local ease_t = t < 0.5 and 4 * t * t * t or 1 - math.pow(-2 * t + 2, 3) / 2

      local cur_x = start_x + (x - start_x) * ease_t
      local cur_y = start_y + (y - start_y) * ease_t

      ura.api.move_window(self.id, math.floor(cur_x), math.floor(cur_y))

      if t >= 1.0 and tb.move_timer then
        ura.api.clear_interval(tb.move_timer)
        tb.move_timer = nil
      end
    end, 1000 / fps)
  end)
end

--- @param width integer
--- @param height integer
--- @param opt { duration?: integer, fps?: number }|nil
function UraWindow:resize(width, height, opt)
  if self:userdata().resize_timer then
    ura.api.clear_interval(self:userdata().resize_timer)
  end

  local geo = self:geometry()
  assert(geo)
  local start_w = geo.width
  local start_h = geo.height

  self:update_userdata(function(t)
    t.geometry = t.geometry or {}
    t.geometry.width = width
    t.geometry.height = height
  end)

  local duration = opt and opt.duration or 0
  local fps = opt and opt.fps or 60

  if duration <= 0 then
    ura.api.resize_window(self.id, width, height)
    return
  end

  self:update_userdata(function(tb)
    local start_time = ura.api.time_since_epoch() / 1e6

    tb.resize_timer = ura.api.set_interval(function()
      local now = ura.api.time_since_epoch() / 1e6
      local elapsed = now - start_time
      local t = math.min(elapsed / duration, 1.0)

      local ease_t = t < 0.5 and 4 * t * t * t or 1 - math.pow(-2 * t + 2, 3) / 2

      local cur_w = start_w + (width - start_w) * ease_t
      local cur_h = start_h + (height - start_h) * ease_t

      ura.api.resize_window(self.id, math.floor(cur_w), math.floor(cur_h))

      if t >= 1.0 and tb.resize_timer then
        ura.api.clear_interval(tb.resize_timer)
        tb.resize_timer = nil
      end
    end, 1000 / fps)
  end)
end

--- @param opt { duration?: integer, fps?: number }|nil
function UraWindow:center(opt)
  local geo = self:geometry()
  assert(geo)
  local o_geo = self:output():logical_geometry()
  assert(o_geo)
  local target_x = o_geo.x + math.floor((o_geo.width - geo.width) / 2)
  local target_y = o_geo.y + math.floor((o_geo.height - geo.height) / 2)
  self:move(target_x, target_y, opt)
end

---@return integer|nil
function UraWindow:lru()
  return ura.api.get_window_lru(self.id)
end

--- @param tags table<string>
function UraWindow:set_tags(tags)
  tags = ura.fn.natural_sort(tags)
  ura.api.set_window_tags(self.id, tags)
end

---@return table<string>
function UraWindow:tags()
  return ura.api.get_window_tags(self.id) or {}
end

---@return table
function UraWindow:userdata()
  return ura.api.get_userdata(self.id) or {}
end

---@param tbl table
function UraWindow:set_userdata(tbl)
  ura.api.set_userdata(self.id, tbl)
end

---@param f fun(t: table)
function UraWindow:update_userdata(f)
  local userdata = self:userdata() or {}
  f(userdata)
  self:set_userdata(userdata)
end

---@return string|nil
function UraWindow:layout()
  local userdata = self:userdata()
  return userdata and userdata.layout or nil
end

---@param layout string
function UraWindow:set_layout(layout)
  local old_layout = self:layout()
  if old_layout == layout then
    return
  end
  self:update_userdata(function(t)
    t.layout = layout
  end)
  ura.hook.emit("window-layout-change", {
    id = self.id,
    from = old_layout,
    to = layout,
  })
end

---@param layout string
function UraWindow:toggle_layout(layout)
  local old = self:layout()
  if old ~= layout then
    self:set_layout(layout)
  else
    self:set_layout(ura.opt["default_layout"] or "tiling")
  end
end

return UraWindow
