--- @meta

--- @class ura.fn
ura.fn = {
  ---@param dst table
  ---@param src table
  merge = function(dst, src) end,
  ---@param id integer
  ---@param tbl table
  update_userdata = function(id, tbl) end,
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
}
