print("add a hook named test: ")

ura.hook.set("window-new", function(_) end, { ns = "test" })

print(ura.api.to_json(ura.hook.HOOKS))

print("remove a hook named test: ")

ura.hook.remove("test")

print(ura.api.to_json(ura.hook.HOOKS))
