local M = {}

M._hooks = {}
M._HOOKS = {}

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

  if M._HOOKS[name] == nil then
    M._HOOKS[name] = {}
    M._hooks[name] = function(e)
      local results = {}
      for _, v in ipairs(M._HOOKS[name]) do
        local success, result = pcall(function()
          return v.func(e)
        end)
        if success then
          table.insert(results, result)
        end
      end
      return results
    end
  end

  table.insert(M._HOOKS[name], hook)

  table.sort(M._HOOKS[name], function(h1, h2)
    return h1.priority < h2.priority
  end)
end

---@param ns string
function M.remove(ns)
  for name, hook in pairs(M._HOOKS) do
    M._HOOKS[name] = ura.fn.filter(hook, function(_, value)
      return value.ns ~= ns
    end)
    if #M._HOOKS[name] == 0 then
      M._HOOKS[name] = nil
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
