--- @meta

--- @class ura.layout
ura.layout = {
  ---@type table
  LAYOUTS = {},
  ---@param id integer
  apply = function(id) end,
  ---@param id integer
  enter = function(id) end,
  ---@param id integer
  leave = function(id) end,
  ---@param name string
  ---@param opt table
  register = function(name, opt) end,
  ---@param win integer
  ---@param layout string
  set = function(win, layout) end,
  ---@param win integer
  ---@return string|nil
  get = function(win) end,
  ---@param id integer
  ---@param layout string
  toggle = function(id, layout) end,
  ---@param id integer
  apply_workspace = function(id) end,
}
