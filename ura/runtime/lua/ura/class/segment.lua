--- @class UraSegment
--- @field label string
local UraSegment = {}
UraSegment.__index = UraSegment

function UraSegment.__eq(a, b)
  return a.label == b.label
end

---@param label string
---@return UraSegment
function UraSegment:new(label)
  local instance = setmetatable({}, self)
  instance.label = label
  return instance
end

--- @param block UraBlock
--- @return UraSegment
function UraSegment:from_block(block)
  return self:new(block.label)
end

--- @return table<UraBlock>
function UraSegment:blocks()
  local all_blocks = ura.class.UraBlock:all()
  local results = {}
  for _, b in ipairs(all_blocks) do
    if b.label == self.label then
      table.insert(results, b)
    end
  end
  return results
end

--- @return table<UraSegment>
function UraSegment:all()
  local blocks = ura.class.UraBlock:all()
  local labels = {}
  local segments = {}

  for _, b in ipairs(blocks) do
    if not labels[b.label] then
      labels[b.label] = true
      table.insert(segments, self:new(b.label))
    end
  end

  table.sort(segments, function(a, b)
    return ura.fn.natural_compare(a.label, b.label)
  end)
  return segments
end

--- @return UraSegment|nil
function UraSegment:current()
  local curr_tag = ura.class.UraOutput:current():tags()[1]
  if not curr_tag then
    return nil
  end
  local b = ura.class.UraBlock:from_tag(curr_tag)
  if not b then
    return nil
  end
  return self:from_block(b)
end

return UraSegment
