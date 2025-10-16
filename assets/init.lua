ura.api.set_keymap("super+t", "normal", function()
	ura.api.spawn("foot")
end)

ura.api.set_keymap("super+q", "normal", function()
	local win = ura.api.get_current_window()
	if win then
		ura.api.close_window(win)
	end
end)

ura.api.set_keymap("super+shift+r", "normal", function()
	ura.api.reload()
end)

ura.api.set_keymap("super+shift+e", "normal", function()
	ura.api.terminate()
end)
ura.api.set_hook("prepare", function() end)
ura.api.set_hook("startup", function() end)
ura.api.set_hook("reload", function() end)
