local M = {}

---@param id integer
function M.close(id)
	ura.api.close_window(id)
end

---@param id integer
function M.center(id)
	local output = ura.api.get_window_output(id)
	assert(output)
	local output_geo = ura.api.get_output_logical_geometry(output)
	assert(output_geo)
	local win_geo = ura.api.get_window_geometry(id)
	assert(win_geo)
	local x = output_geo.x + (output_geo.width - win_geo.width) / 2
	local y = output_geo.y + (output_geo.height - win_geo.height) / 2
	ura.api.move_window(id, x, y)
end

return M
