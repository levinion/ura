local M = {}

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

---@param str string
---@param sep string
---@return table
function M.split(str, sep)
  local result = {}
  local start = 1
  local split_start, split_end = string.find(str, sep, start, true)
  while split_start do
    table.insert(result, string.sub(str, start, split_start - 1))
    start = split_end + 1
    split_start, split_end = string.find(str, sep, start, true)
  end
  table.insert(result, string.sub(str, start))
  return result
end

---@param tbl table
---@param f fun(index: integer, value: any):boolean
---@return table
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

---@param a string
---@param b string
---@return boolean
function M.natural_compare(a, b)
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
end

---@param t table
---@return table
function M.natural_sort(t)
  table.sort(t, M.natural_compare)
  return t
end

---@param t table
---@param f fun(v:any):boolean
---@return integer|nil
function M.find(t, f)
  for i, v in ipairs(t) do
    if f(v) then
      return i
    end
  end
  return nil
end

---@param t table
---@return table
function M.unique(t)
  local checked = {}
  local tb = {}
  for _, v in ipairs(t) do
    if not checked[v] then
      checked[v] = true
      table.insert(tb, v)
    end
  end
  return tb
end

---@param opt table|nil
---@return table<string>
function M.collect_tags(opt)
  opt = opt or { include_active = true }

  local tags = {}
  for _, w in ipairs(ura.class.UraWindow:all()) do
    for _, tag in ipairs(w:tags()) do
      table.insert(tags, tag)
    end
  end

  if opt.include_active then
    local active_tags = ura.class.UraOutput:current():tags()
    for _, tag in ipairs(active_tags) do
      table.insert(tags, tag)
    end
  end

  return ura.fn.natural_sort(ura.fn.unique(tags))
end

---@param pattern string
---@param s string
---@param flags integer|nil
---@return boolean
function M.fnmatch(pattern, s, flags)
  local ffi = require("ffi")
  ffi.cdef([[
    int fnmatch(const char *pattern, const char *string, int flags);
]])
  return ffi.C.fnmatch(pattern, s, flags or 0) == 0
end

---@param path string
function M.exists(path)
  if not path then
    return false
  end
  local f = io.open(path, "r")
  if f then
    f:close()
    return true
  end
  return false
end

function M.find_config_path()
  local xdg_config = os.getenv("XDG_CONFIG_HOME")
  local home = os.getenv("HOME")
  local root = nil

  if xdg_config and xdg_config ~= "" then
    root = xdg_config
  elseif home then
    root = home .. "/.config"
  end

  if root ~= nil then
    local dotfile = root .. "/ura/init.lua"
    if ura.fn.exists(dotfile) then
      return dotfile
    end
  end

  local global_dotfile = "/etc/ura/init.lua"
  if ura.fn.exists(global_dotfile) then
    return global_dotfile
  end

  return nil
end

local context

function M._save_context()
  context = {
    loaded = {},
    opt = ura.fn.copy(ura.opt),
    keymap = { _KEYMAPS = ura.fn.copy(ura.keymap._KEYMAPS), _keymaps = ura.fn.copy(ura.keymap._keymaps) },
    hook = { _HOOKS = ura.fn.copy(ura.hook._HOOKS), _hooks = ura.fn.copy(ura.hook._hooks) },
  }

  for k, v in pairs(package.loaded) do
    context.loaded[k] = v
  end
end

function M._restore_context()
  for k in pairs(package.loaded) do
    package.loaded[k] = nil
  end
  for k, v in pairs(context.loaded) do
    package.loaded[k] = v
  end

  ura.opt = ura.fn.copy(context.opt)
  ura.keymap._KEYMAPS = ura.fn.copy(context.keymap._KEYMAPS)
  ura.keymap._keymaps = ura.fn.copy(context.keymap._keymaps)
  ura.hook._HOOKS = ura.fn.copy(context.hook._HOOKS)
  ura.hook._hooks = ura.fn.copy(context.hook._hooks)
end

---@return boolean
---@return function|string
function M._load_config()
  local path = M.find_config_path()
  if not path then
    return false, "could not find any config files, exiting..."
  end

  local chunk, err = loadfile(path)
  if not chunk then
    return false, "Syntax Error: " .. (err or "unknown")
  end

  return true, chunk
end

---@param chunk function
---@return boolean
---@return string|nil
function M._safe_call(chunk)
  local new_env = {}
  setmetatable(new_env, { __index = _G })
  setfenv(chunk, new_env)
  local success, run_err = pcall(chunk)
  if not success then
    return false, "Runtime Error: " .. (run_err or "unknown")
  end
  return true
end

function M.rshift(v, n)
  local ffi = require("ffi")
  return ffi.new("uint64_t", v) / (2 ^ n)
end

function M.lshift(v, n)
  local ffi = require("ffi")
  return ffi.new("uint64_t", v) * (2 ^ n)
end

-- thanks to http://lua-users.org/wiki/CopyTable
function M.copy(orig, copies)
  copies = copies or {}
  local orig_type = type(orig)
  local copy
  if orig_type == "table" then
    if copies[orig] then
      copy = copies[orig]
    else
      copy = {}
      copies[orig] = copy
      for orig_key, orig_value in next, orig, nil do
        copy[M.copy(orig_key, copies)] = M.copy(orig_value, copies)
      end
      setmetatable(copy, M.copy(getmetatable(orig), copies))
    end
  else -- number, string, boolean, etc
    copy = orig
  end
  return copy
end

return M
