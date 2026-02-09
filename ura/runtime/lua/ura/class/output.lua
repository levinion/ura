--- @class UraOutput
--- @field id integer
local UraOutput = {}
UraOutput.__index = UraOutput

function UraOutput.__eq(a, b)
  if rawequal(a, b) then
    return true
  end
  if type(a) == "table" and type(b) == "table" then
    return a.id == b.id
  end
  return false
end

---@param id integer
---@return UraOutput
function UraOutput:new(id)
  local instance = setmetatable({}, self)
  instance.id = id
  return instance
end

---@return UraOutput|nil
function UraOutput:current()
  local id = ura.api.get_current_output()
  return id and UraOutput:new(id) or nil
end

---@return table<UraOutput>
function UraOutput:all()
  local outputs = ura.api.get_all_outputs()
  local t = {}
  for _, output in ipairs(outputs) do
    table.insert(t, ura.class.UraOutput:new(output))
  end
  return t
end

---@param name string
---@return UraOutput|nil
function UraOutput:from_name(name)
  local id = ura.api.get_output(name)
  return id and UraOutput:new(id) or nil
end

---@return string|nil
function UraOutput:name()
  return ura.api.get_output_name(self.id)
end

---@param flag boolean
function UraOutput:set_dpms(flag)
  ura.api.set_output_dpms(self.id, flag)
end

---@return table|nil
function UraOutput:logical_geometry()
  return ura.api.get_output_logical_geometry(self.id)
end

---@return table|nil
function UraOutput:usable_geometry()
  return ura.api.get_output_usable_geometry(self.id)
end

---@return number|nil
function UraOutput:scale()
  return ura.api.get_output_scale(self.id)
end

---@param tags table<string>
function UraOutput:set_tags(tags)
  tags = ura.fn.natural_sort(tags)
  ura.api.set_output_tags(self.id, tags)
end

---@return table<string>
function UraOutput:tags()
  return ura.api.get_output_tags(self.id) or {}
end

---@return table
function UraOutput:userdata()
  return ura.api.get_userdata(self.id)
end

---@param tbl table
function UraOutput:set_userdata(tbl)
  ura.api.set_userdata(self.id, tbl)
end

---@param tbl table
function UraOutput:update_userdata(tbl)
  local userdata = self:userdata() or {}
  ura.fn.merge(userdata, tbl)
  self:set_userdata(userdata)
end

return UraOutput
