local M = {}

M._hooks = {}

local HOOKS = {}

---@return table
function M.all()
  return HOOKS
end

---@param name string
---@param f fun(e: table):any
---@param opt table|nil
function M.add(name, f, opt)
  local hook = { func = f }
  if opt ~= nil and ura.fn.validate(opt, "ns", "string") then
    hook.ns = opt.ns
  else
    hook.ns = "default"
  end
  if opt ~= nil and ura.fn.validate(opt, "priority", "number") then
    hook.priority = opt.priority
  else
    hook.priority = 50
  end
  if opt ~= nil and ura.fn.validate(opt, "desc", "string") then
    hook.desc = opt.desc
  end

  if HOOKS[name] == nil then
    HOOKS[name] = {}
    M._hooks[name] = function(e)
      for _, v in ipairs(HOOKS[name]) do
        v.func(e)
      end
    end
  end

  table.insert(HOOKS[name], hook)

  table.sort(HOOKS[name], function(h1, h2)
    return h1.priority < h2.priority
  end)
end

---@param ns string
function M.remove(ns)
  for name, hook in pairs(HOOKS) do
    HOOKS[name] = ura.fn.filter(hook, function(_, value)
      return value.ns ~= ns
    end)
    if #HOOKS[name] == 0 then
      HOOKS[name] = nil
      M._hooks[name] = nil
    end
  end
end

---@param name string
---@param args table|nil
function M.emit(name, args)
  M._hooks[name](args)
end

return M
