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
  local instance = setmetatable({}, UraSegment)
  instance.label = label
  return instance
end

--- @param tag string
--- @return UraSegment|nil
function UraSegment:from_tag(tag)
  local block = ura.class.UraBlock:from_tag(tag)
  return block and self:from_block(block) or nil
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

--- @return table<UraWindow>
function UraSegment:windows()
  local wins = ura.class.UraWindow:all()
  local t = {}
  for _, win in ipairs(wins) do
    if
      ura.fn.find(win:tags(), function(v)
        local block = ura.class.UraBlock:from_tag(v)
        if block then
          return block.label == self.label
        end
        return false
      end)
    then
      table.insert(t, win)
    end
  end
  return t
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

--- @return table<UraSegment>
function UraSegment:current()
  local segs = {}
  for _, tag in ipairs(ura.class.UraOutput:current():tags()) do
    local seg = self:from_tag(tag)
    if seg then
      segs[seg.label] = seg
    end
  end
  local ss = {}
  for _, s in pairs(segs) do
    table.insert(ss, s)
  end
  table.sort(ss, function(a, b)
    return ura.fn.natural_compare(a.label, b.label)
  end)
  return ss
end

function UraSegment:active_block()
  local blocks = self:blocks()
  if #blocks == 0 then
    return nil
  end
  if #blocks == 1 then
    return blocks[1]
  end

  local active_block = nil
  local max_lru = -1

  for _, block in ipairs(blocks) do
    local wins = block:windows()
    for _, win in ipairs(wins) do
      local lru = win:lru() or 0
      if lru > max_lru then
        max_lru = lru
        active_block = block
      end
    end
  end

  return active_block
end

return UraSegment
