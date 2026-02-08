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
  local instance = setmetatable({}, self)
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
      if ura.fn.icontains(w:tags(), tag) then
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
  return ura.api.get_window_geometry(self.id)
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
function UraWindow:is_draggable()
  return ura.api.is_window_draggable(self.id)
end

--- @param flag boolean
function UraWindow:set_draggable(flag)
  ura.api.set_window_draggable(self.id, flag)
end

---@return boolean|nil
function UraWindow:is_fullscreen()
  return ura.api.is_window_fullscreen(self.id)
end

--- @param flag boolean
function UraWindow:set_fullscreen(flag)
  ura.api.set_window_fullscreen(self.id, flag)
end

--- @param x integer
--- @param y integer
function UraWindow:move(x, y)
  ura.api.move_window(self.id, x, y)
end

--- @param width integer
--- @param height integer
function UraWindow:resize(width, height)
  ura.api.resize_window(self.id, width, height)
end

function UraWindow:center()
  local geo = self:geometry()
  assert(geo)
  local o_geo = self:output():logical_geometry()
  assert(o_geo)
  local target_x = o_geo.x + math.floor((o_geo.width - geo.width) / 2)
  local target_y = o_geo.y + math.floor((o_geo.height - geo.height) / 2)
  self:move(target_x, target_y)
end

--- @param tags table<string>
function UraWindow:set_tags(tags)
  ura.api.set_window_tags(self.id, tags)
end

---@return table<string>
function UraWindow:tags()
  return ura.api.get_window_tags(self.id) or {}
end

---@return table
function UraWindow:userdata()
  return ura.api.get_userdata(self.id)
end

---@param tbl table
function UraWindow:set_userdata(tbl)
  ura.api.set_userdata(self.id, tbl)
end

---@param tbl table
function UraWindow:update_userdata(tbl)
  local userdata = self:userdata() or {}
  ura.fn.merge(userdata, tbl)
  self:set_userdata(userdata)
end

---@return string|nil
function UraWindow:layout()
  return ura.layout.get(self)
end

---@param layout string
function UraWindow:set_layout(layout)
  ura.layout.set(self, layout)
end

function UraWindow:apply_layout()
  ura.layout.apply(self)
end

---@param layout string
function UraWindow:toggle_layout(layout)
  local old = self:layout()
  if old ~= layout then
    self:set_layout(layout)
  else
    self:set_layout(ura.opt["layout.default"] or "tiling")
  end
end

return UraWindow
