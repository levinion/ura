local M = {}

---@param src table
---@param dst table
function M.merge(dst, src)
  for k, v in pairs(src) do
    dst[k] = v
  end
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

---@param path string
function M.load(path)
  ura.api.prepend_package_path(ura.api.expand(path .. "/lua/?/init.lua"))
  ura.api.prepend_package_path(ura.api.expand(path .. "/lua/?.lua"))
end

---@param path string
function M.load_dir(path)
  local output = ura.fn.shell("find " .. path .. " -maxdepth 1")
  local plugins = ura.fn.split(output, "\n")
  for _, plugin in ipairs(plugins) do
    ura.api.prepend_package_path(plugin .. "/lua/?/init.lua")
    ura.api.prepend_package_path(plugin .. "/lua/?.lua")
  end
end

---@param t table
---@param value any
---@return boolean
function M.icontains(t, value)
  for _, v in ipairs(t) do
    if v == value then
      return true
    end
  end
  return false
end

---@param t table
---@param value any
---@return boolean
function M.contains(t, value)
  return t[value] ~= nil
end

---@param t table
---@return table
function M.natural_sort(t)
  table.sort(t, function(a, b)
    local function get_priority(s)
      s = tostring(s)
      local has_digit = s:find("%d") ~= nil
      local has_non_digit = s:find("%D") ~= nil

      if has_digit and not has_non_digit then
        return 1
      elseif has_digit and has_non_digit then
        return 2
      else
        return 3
      end
    end

    local pa = get_priority(a)
    local pb = get_priority(b)

    if pa ~= pb then
      return pa < pb
    end

    local sa, sb = tostring(a), tostring(b)

    local function tokenize(str)
      local tokens = {}
      for digit, non_digit in str:gmatch("(%d*)(%D*)") do
        if digit ~= "" then
          table.insert(tokens, tonumber(digit))
        end
        if non_digit ~= "" then
          table.insert(tokens, non_digit)
        end
      end
      return tokens
    end

    local ta = tokenize(sa)
    local tb = tokenize(sb)

    for i = 1, math.max(#ta, #tb) do
      local va, vb = ta[i], tb[i]
      if va == nil then
        return true
      end
      if vb == nil then
        return false
      end

      if type(va) ~= type(vb) then
        return type(va) == "number"
      elseif va ~= vb then
        return va < vb
      end
    end
    return false
  end)
  return t
end

---@param t table
---@param value any
---@return integer|nil
function M.find(t, value)
  for i, v in ipairs(t) do
    if v == value then
      return i
    end
  end
  return nil
end

---@param opt table|nil
---@return table<string>
function M.collect_tags(opt)
  opt = opt or { include_active = true }

  local tags = {}
  local checked = {}
  for _, w in ipairs(ura.class.UraWindow:all()) do
    for _, tag in ipairs(w:tags()) do
      if not checked[tag] then
        checked[tag] = true
        table.insert(tags, tag)
      end
    end
  end

  if opt.include_active then
    local active_tags = ura.class.UraOutput:current():tags()
    for _, tag in ipairs(active_tags) do
      if not checked[tag] then
        checked[tag] = true
        table.insert(tags, tag)
      end
    end
  end

  return ura.fn.natural_sort(tags)
end

return M
