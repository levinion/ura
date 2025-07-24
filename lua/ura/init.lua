local home_dir = os.getenv("HOME")
package.path = package.path .. ";" .. home_dir .. "/.config/ura/lua/?.lua"

local M = {}

---@param modifiers string
---@param key string
---@param f fun()
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

---@param rate integer
---@param delay number
function M.set_keyboard_repeat(rate, delay)
  _ura.set_keyboard_repeat(rate, delay)
end

---@param index integer
---@return number
function M.switch_workspace(index)
  return _ura.switch_workspace(index)
end

---@param index integer
---@return number
function M.move_to_workspace(index)
  return _ura.move_to_workspace(index)
end

---workspace index starts with 0
---@return integer
function M.current_workspace()
  return _ura.current_workspace()
end

---@param name string
---@param f function
function M.hook(name, f)
  _ura.hook(name, f);
end

---@param options {inner: number, outer_l: number, outer_r: number, outer_t: number, outer_b: number}
function M.tiling_gap(options)
  _ura.tiling_gap(
    options.inner or 10,
    options.outer_l or 10,
    options.outer_r or 10,
    options.outer_t or 10,
    options.outer_b or 10
  )
end

---if theme is empty, then fallback to default
---@param theme string
---@param size integer
function M.set_cursor_theme(theme, size)
  _ura.set_cursor_theme(theme, size)
end

---@return number
function M.current_toplevel()
  return _ura.current_toplevel()
end

---@param index integer
---@return boolean
function M.focus(index)
  return _ura.focus(index)
end

---@param name string
function M.set_cursor_shape(name)
  _ura.set_cursor_shape(name)
end

---@param x integer
---@param y integer
function M.cursor_relative_move(x, y)
  _ura.cursor_relative_move(x, y)
end

---@param x integer
---@param y integer
function M.cursor_absolute_move(x, y)
  _ura.cursor_absolute_move(x, y)
end

return M
