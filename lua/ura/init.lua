local M = {}

---comment
---@param modifiers string
---@param key string
---@param f function
function M.map(modifiers, key, f)
  _ura.map(modifiers, key, f)
end

---comment
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

---comment
---@param scale number
function M.set_output_scale(scale)
  _ura.set_output_scale(scale)
end

---comment
---@param rate number
---@param delay number
function M.set_keyboard_repeat(rate, delay)
  _ura.set_keyboard_repeat(rate, delay)
end

return M
