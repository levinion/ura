--- @class UraBlock
--- @field label string
--- @field index integer
local UraBlock = {}
UraBlock.__index = UraBlock

function UraBlock.__eq(a, b)
  if rawequal(a, b) then
    return true
  end
  if type(a) == "table" and type(b) == "table" then
    return a.label == b.label and a.index == b.index
  end
  return false
end

---@param label string
---@param index integer
---@return UraBlock
function UraBlock:new(label, index)
  local instance = setmetatable({}, self)
  instance.label = label
  instance.index = index
  instance.lru = 0
  return instance
end

---@param tag string
---@return UraBlock|nil
function UraBlock:from_tag(tag)
  local t = ura.fn.split(tag, ":")
  if #t < 2 then
    return nil
  end
  local index = tonumber(t[#t])
  if index == nil then
    return nil
  end
  local label = table.concat(t, ":", 1, #t - 1)
  return self:new(label, index)
end

---@return UraBlock|nil
function UraBlock:current()
  return self:from_tag(ura.class.UraOutput:current():tags()[1])
end

---@return string
function UraBlock:tag()
  return self.label .. ":" .. tostring(self.index)
end

---@return table<UraBlock>
function UraBlock:all()
  local segs = {}
  local tags = ura.fn.collect_tags()
  for _, tag in ipairs(tags) do
    local seg = self:from_tag(tag)
    if seg ~= nil then
      table.insert(segs, seg)
    end
  end
  table.sort(segs, function(a, b)
    return ura.fn.natural_compare(a:tag(), b:tag())
  end)
  return segs
end

---@return UraSegment|nil
function UraBlock:segment()
  for _, v in ura.class.UraSegment:all() do
    if v.label == self.label then
      return v
    end
  end
  return nil
end

return UraBlock
