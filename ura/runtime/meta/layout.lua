--- @meta

--- @class ura.layout
ura.layout = {
  ---@type table
  LAYOUTS = {},
  ---@param win UraWindow
  apply = function(win) end,
  ---@param win UraWindow
  enter = function(win) end,
  ---@param win UraWindow
  leave = function(win) end,
  ---@param name string
  ---@param opt table
  register = function(name, opt) end,
  ---@param win UraWindow
  ---@param layout string
  set = function(win, layout) end,
  ---@param win UraWindow
  ---@return string|nil
  get = function(win) end,
}
