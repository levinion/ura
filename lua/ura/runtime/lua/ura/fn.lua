local M = {}

---@param src table
---@param dst table
function M.merge(dst, src)
  for k, v in pairs(src) do
    dst[k] = v
  end
end

---@param id integer
---@param tbl table
function M.update_userdata(id, tbl)
  local userdata = ura.api.get_userdata(id) or {}
  ura.fn.merge(userdata, tbl)
  ura.api.set_userdata(id, userdata)
end

---@param variable any
---@param path string
---@param typ type
---@return boolean
function M.validate(variable, path, typ)
  if type(variable) ~= "table" then
    return false
  end
  local array = M.split(path, ":")
  if #array == 1 then
    return type(variable[array[1]]) == typ
  end
  local current = variable
  for i, v in ipairs(array) do
    if type(current) ~= "table" then
      return false
    end
    if i == #array then
      return type(current[v]) == typ
    end
    local next_value = current[v]
    if next_value == nil then
      return false
    end
    if type(next_value) ~= "table" then
      return false
    end
    current = next_value
  end
  return false
end

--- thanks: http://lua-users.org/wiki/SplitJoin
---@param str string
---@param pat string
---@return table
function M.split(str, pat)
  local t = {} -- NOTE: use {n = 0} in Lua-5.0
  local fpat = "(.-)" .. pat
  local last_end = 1
  local s, e, cap = str:find(fpat, 1)
  while s do
    if s ~= 1 or cap ~= "" then
      table.insert(t, cap)
    end
    last_end = e + 1
    s, e, cap = str:find(fpat, last_end)
  end
  if last_end <= #str then
    cap = str:sub(last_end)
    table.insert(t, cap)
  end
  return t
end

---@param tbl table
---@param f fun(index: integer, value: any):table
function M.filter(tbl, f)
  local r = {}
  for i, v in ipairs(tbl) do
    if f(i, v) == true then
      table.insert(r, v)
    end
  end
  return r
end

---@param cmd string
---@return string
function M.shell(cmd)
  local pfile = assert(io.popen(cmd, "r"))
  local output = pfile:read("*a")
  pfile:close()
  return output
end

return M
