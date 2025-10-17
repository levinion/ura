local json = ura.api.to_json({ 1, 2, 3, { pi = 3.14 } })
print(json, type(json))

print(ura.api.parse_json("true"))
print(ura.api.parse_json("123"))
print(ura.api.parse_json("3.14"))

local data = ura.api.parse_json('{"pi" : 3.14}')
print(data, type(data))
for k, v in pairs(data) do
	print(k, type(k), v, type(v))
end
print(data.pi, type(data.pi))

local array = ura.api.parse_json("[1,2,3]")
print(array, type(array))
for _, v in ipairs(array) do
	print(v, type(v))
end

local str = ura.api.parse_json('"hello world"')
print(str, type(str))

local output = ura.api.get_current_output()
assert(output)
local geo = ura.api.get_output_logical_geometry(output)
print(geo, ura.api.to_json(geo))
