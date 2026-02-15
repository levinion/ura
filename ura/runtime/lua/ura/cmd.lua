local M = {}

local function get_distance(current_geo, target_geo, direction)
  local dx = (current_geo.x + current_geo.width / 2) - (target_geo.x + target_geo.width / 2)
  local dy = (current_geo.y + current_geo.height / 2) - (target_geo.y + target_geo.height / 2)
  if direction == "left" then
    return dx
  elseif direction == "right" then
    return -dx
  elseif direction == "up" then
    return dy
  elseif direction == "down" then
    return -dy
  end
  return math.huge
end

local function focus_in_direction(direction)
  local w = ura.class.UraWindow:current()
  assert(w)

  if w:layout() == "fullscreen" then
    return
  end

  local geo = w:geometry()
  local closest_win = nil
  local min_dist = math.huge

  for _, win in ipairs(ura.class.UraWindow:from_tags(w:output():tags())) do
    if win ~= w then
      local t_geo = win:geometry()
      local dist = get_distance(geo, t_geo, direction)

      if dist >= 0 then
        if dist < min_dist then
          min_dist = dist
          closest_win = win
        end
      end
    end
  end

  if closest_win then
    closest_win:focus()
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
    ura.opt = {}
    ura.hook._reset()
    ura.keymap._reset()
    ura.fn._restore_context()
    --[[@diagnostic disable-next-line: param-type-mismatch]]
    local status2, err = ura.fn._safe_call(v)
    if not status2 then
      error(err)
    end
  end
end

return M
