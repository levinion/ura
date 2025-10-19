local M = {}

M.HOOKS = {}

---@param name string
---@param f function
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
  if opt ~= nil and ura.fn.validate(opt, "enabled", "boolean") then
    hook.enabled = opt.enabled
  else
    hook.enabled = true
  end
  if type(M.HOOKS[name]) ~= "table" then
    M.HOOKS[name] = {}
  end

  local exist = false

  for _, v in ipairs(M.HOOKS[name]) do
    if v.ns == hook.ns then
      v = hook
      exist = true
      break
    end
  end

  if not exist then
    table.insert(M.HOOKS[name], hook)
  end

  table.sort(M.HOOKS[name], function(h1, h2)
    return h1.priority < h2.priority
  end)

  ura.api.set_hook(name, function(e)
    for _, v in ipairs(M.HOOKS[name]) do
      if v.enabled then
        v.func(e)
      end
    end
  end)
end

---@param ns string
function M.disable(ns)
  for _, hook in pairs(M.HOOKS) do
    if hook.ns == ns then
      hook.enabled = false
    end
  end
end

---@param ns string
function M.enable(ns)
  for _, hook in pairs(M.HOOKS) do
    if hook.ns == ns then
      hook.enabled = true
    end
  end
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
