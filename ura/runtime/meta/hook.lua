--- @meta

--- @class ura.hook
ura.hook = {
  ---@type table
  HOOK = {},
  ---@param name string
  ---@param f fun(e: table):any
  ---@param opt table|nil
  add = function(name, f, opt) end,
  ---@param ns string
  remove = function(ns) end,
  ---@param name string
  ---@param args table|nil
  emit = function(name, args) end,
}
