local s = ura.fn.split("a:b:c", ":")
assert(ura.api.to_json(s), '["a","b","c"]')

local test = {
  a = { b = { c = 3.14 } },
}
assert(ura.fn.validate(test, "a", "table"))
assert(ura.fn.validate(test, "a:b", "table"))
assert(ura.fn.validate(test, "a:b:c", "number"))

print("assert success")
