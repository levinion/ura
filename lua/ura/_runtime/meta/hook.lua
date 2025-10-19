--- @meta

--- @class ura.hook
ura.hook = {
  ---@type table
  HOOK = {},
  ---@param name string
  ---@param f function
  ---@param opt table|nil
  set = function(name, f, opt) end,
}
