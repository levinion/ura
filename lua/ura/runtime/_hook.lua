local M = {}

---@param name string
---@param f function
function M.set(name, f)
	ura.api.set_hook(name, f)
end

return M
