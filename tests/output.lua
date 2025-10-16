local output = ura.api.get_current_output()

if not output then
	return
end

local geo = ura.api.get_output_logical_geometry(output)

print(geo.x, geo.y, geo.width, geo.height)
