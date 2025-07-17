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

function M.prev_workspace()
  _ura.prev_workspace();
end

function M.next_workspace()
  _ura.next_workspace();
end

---@param name string
---@param f function
function M.hook(name, f)
  _ura.hook(name, f);
end

---@param gap number
function M.tiling_gap(gap)
  _ura.tiling_gap(gap)
end

return M
