local M = {}

M.HOOKS = {}

---@param name string
---@param f fun(e: table):any
---@param opt table|nil
function M.set(name, f, opt)
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
  if type(M.HOOKS[name]) ~= "table" then
    M.HOOKS[name] = {}
    ura.api.set_hook(name, function(e)
      for _, v in ipairs(M.HOOKS[name]) do
        v.func(e)
      end
    end)
  end

  table.insert(M.HOOKS[name], hook)

  table.sort(M.HOOKS[name], function(h1, h2)
    return h1.priority < h2.priority
  end)
end

---@param ns string
function M.remove(ns)
  for name, hook in pairs(M.HOOKS) do
    M.HOOKS[name] = ura.fn.filter(hook, function(_, value)
      return value.ns ~= ns
    end)
    ura.api.set_hook(name, function(e)
      for _, v in ipairs(M.HOOKS[name]) do
        if v.enabled then
          v.func(e)
        end
      end
    end)
  end
end

return M
