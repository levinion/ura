local M = {}

local function focus_in_direction(direction)
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
  if w:layout() == "fullscreen" then
    return
  end
  local geo = w:output():logical_geometry()
  local candidates = {}
  for i, win in ipairs(ura.class.UraWindow:from_tags(w:output():tags())) do
    table.insert(candidates, { index = i, instance = win, dist = get_distance(win:geometry(), geo) })
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

function M.focus_left()
  focus_in_direction("left")
end

function M.focus_right()
  focus_in_direction("right")
end

function M.focus_up()
  focus_in_direction("up")
end

function M.focus_down()
  focus_in_direction("down")
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
