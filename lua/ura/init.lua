local M = {}

---@param modifiers string
---@param key string
---@param f function
function M.map(modifiers, key, f)
  _ura.map(modifiers, key, f)
end

---@param name string
---@param value string
function M.env(name, value)
  _ura.env(name, value)
end

function M.fullscreen()
  _ura.fullscreen()
end

function M.terminate()
  _ura.terminate()
end

function M.reload()
  _ura.reload()
end

function M.close_window()
  _ura.close_window()
end

---@param scale number
function M.set_output_scale(scale)
  _ura.set_output_scale(scale)
end

---@param rate number
---@param delay number
function M.set_keyboard_repeat(rate, delay)
  _ura.set_keyboard_repeat(rate, delay)
end

---@param index number
function M.switch_workspace(index)
  _ura.switch_workspace(index)
end

---@param index number
function M.move_to_workspace(index)
  _ura.move_to_workspace(index)
end

---workspace index starts with 0
---@return number
function M.current_workspace()
  return _ura.current_workspace()
end

---@param name string
---@param f function
function M.hook(name, f)
  _ura.hook(name, f);
end

---@param outer number
---@param inner number
function M.tiling_gap(outer, inner)
  _ura.tiling_gap(outer, inner)
end

---if theme is empty, then fallback to default
---@param theme string
---@param size number
function M.cursor_theme(theme, size)
  _ura.cursor_theme(theme, size)
end

---@param refresh number
function M.set_output_refresh(refresh)
  _ura.set_output_refresh(refresh)
end

return M
