local M = {}

---@param opt { blacklist: table<string> }
local function focus_in_direction(direction, opt)
  local blacklist = opt and opt.blacklist or {}
  local function get_distance(current_geo, target_geo)
    local dx = current_geo.x - target_geo.x
    local dy = current_geo.y - target_geo.y
    if direction == "left" or direction == "right" then
      return dx
    elseif direction == "up" or direction == "down" then
      return dy
    end
  end

  local w = ura.class.UraWindow:current()
  assert(w)
  if w:userdata().focus_exclusive == true then
    return
  end
  local geo = w:output():logical_geometry()
  local candidates = {}
  for i, win in ipairs(ura.class.UraWindow:from_tags(w:output():tags())) do
    if
      ura.fn.find(win:tags(), function(v1)
        return ura.fn.find(blacklist, function(v2)
          return v1 == v2
        end) ~= nil
      end) == nil
    then
      table.insert(candidates, { index = i, instance = win, dist = get_distance(win:geometry(), geo) })
    end
  end
  table.sort(candidates, function(a, b)
    return a.dist ~= b.dist and a.dist < b.dist or a.index < b.index
  end)
  local index = ura.fn.find(candidates, function(v)
    return v.instance == w
  end)
  if index < #candidates and (direction == "right" or direction == "down") then
    candidates[index + 1].instance:focus()
  elseif index > 1 and (direction == "left" or direction == "up") then
    candidates[index - 1].instance:focus()
  end
end

---@param opt table
function M.focus_left(opt)
  focus_in_direction("left", opt)
end

---@param opt table
function M.focus_right(opt)
  focus_in_direction("right", opt)
end

---@param opt table
function M.focus_up(opt)
  focus_in_direction("up", opt)
end

---@param opt table
function M.focus_down(opt)
  focus_in_direction("down", opt)
end

function M.reload()
  local status, v = ura.fn._load_config()
  if not status then
    error(v)
  else
    ura.fn._restore_context()
    --[[@diagnostic disable-next-line: param-type-mismatch]]
    local status2, err = ura.fn._safe_call(v)
    if not status2 then
      error(err)
    end
  end
end

return M
