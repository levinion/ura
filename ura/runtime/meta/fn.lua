--- @meta

--- @class ura.fn
ura.fn = {
  ---@param dst table
  ---@param src table
  merge = function(dst, src) end,
  ---@param variable any
  ---@param path string
  ---@param typ type
  ---@return boolean
  validate = function(variable, path, typ) end,
  ---@param str string
  ---@param pat string
  ---@return table
  split = function(str, pat) end,
  ---@param tbl table
  ---@param f fun(index: integer, value: any):boolean
  filter = function(tbl, f) end,
  ---@param cmd string
  ---@return string
  shell = function(cmd) end,
  ---@param path string
  load = function(path) end,
  ---@param path string
  load_dir = function(path) end,
  ---@param t table
  ---@param value any
  ---@return boolean
  icontains = function(t, value) end,
  ---@param t table
  ---@param value any
  ---@return boolean
  contains = function(t, value) end,
  ---@param t table
  ---@return table
  natural_sort = function(t) end,
  ---@param t table
  ---@param value any
  ---@return integer|nil
  find = function(t, value) end,
  ---@param opt table|nil
  ---@return table<string>
  collect_tags = function(opt) end,
}
