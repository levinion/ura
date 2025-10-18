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
	if type(M.HOOKS[name]) ~= "table" then
		M.HOOKS[name] = {}
	end
	table.insert(M.HOOKS[name], hook)
	table.sort(M.HOOKS[name], function(h1, h2)
		return h1.priority < h2.priority
	end)
	ura.api.set_hook(name, function(e)
		for _, v in ipairs(M.HOOKS[name]) do
			v.func(e)
		end
	end)
end

return M
