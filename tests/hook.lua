ura.hook.set("window-new", function(_)
  ura.api.notify("test", "window-new")
end, { ns = "test" })

print(ura.api.to_json(ura.hook.HOOKS))
