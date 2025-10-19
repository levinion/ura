--- @meta

--- @class ura.hook
ura.hook = {
  ---@type table
  HOOK = {},
  ---@param name string
  ---@param f function
  ---@param opt table|nil
  set = function(name, f, opt) end,
  ---@param ns string
  disable = function(ns) end,
  ---@param ns string
  enable = function(ns) end,
  ---@param ns string
  remove = function(ns) end,
}
