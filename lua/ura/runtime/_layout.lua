local M = {}

---@param win integer
function M.toogle_fullscreen(win)
	local is_fullscreen = ura.api.is_window_fullscreen(win)
	assert(is_fullscreen ~= nil)
	local output = ura.api.get_current_output()
	assert(output)
	if not is_fullscreen then
		local geo = ura.api.get_output_logical_geometry(output)
		assert(geo)
		local win_geo = ura.api.get_window_geometry(win)
		assert(win_geo)
		local z_index = ura.api.get_window_z_index(win)
		assert(z_index)
		local draggable = ura.api.is_window_draggable(win)
		assert(draggable ~= nil)
		ura.api.set_userdata(win, {
			x = win_geo.x,
			y = win_geo.y,
			width = win_geo.width,
			height = win_geo.height,
			z_index = z_index,
			draggable = draggable,
		})
		ura.api.set_window_draggable(win, false)
		ura.api.set_window_z_index(win, 500)
		ura.api.resize_window(win, geo.width, geo.height)
		ura.api.move_window(win, geo.x, geo.y)
		ura.api.set_window_fullscreen(win, true)
	else
		local userdata = ura.api.get_userdata(win)
		assert(userdata)
		ura.api.set_window_draggable(win, userdata.draggable or true)
		ura.api.set_window_z_index(win, userdata.z_index or 100)
		ura.api.resize_window(win, userdata.width or 800, userdata.height or 600)
		ura.api.move_window(win, userdata.x or 0, userdata.y or 0)
	end
end

-- ---@param win integer
-- function M.toogle_tiling(win)
-- 	pcall(function()
-- 			ura.api.set_window_draggable(win, false)
-- 			ura.api.set_window_z_index(win, 500)
-- 			ura.api.resize_window(win, geo.width, geo.height)
-- 			ura.api.move_window(win, geo.x, geo.y)
-- 			ura.api.set_window_fullscreen(win, true)
--
-- 			local geo = ura.api.get_output_usable_geometry(output)
-- 			assert(geo)
-- 			ura.api.set_window_draggable(win, true)
-- 			ura.api.set_window_z_index(win, 100)
-- 			ura.api.resize_window(win, geo.width, geo.height)
-- 			ura.api.move_window(win, geo.x, geo.y)
-- 			ura.api.set_window_fullscreen(win, false)
-- 	end)
-- end

return M
