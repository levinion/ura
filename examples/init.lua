ura.api.set_keymap("super+t", "normal", function()
	ura.api.spawn("foot -e tmux")
end)

ura.api.set_keymap("super+w", "normal", function()
	ura.api.spawn("firefox-developer-edtion")
end)

ura.api.set_keymap("super+q", "normal", function()
	local win = ura.api.get_current_window()
	if win then
		ura.api.close_window(win)
	end
end)

ura.api.set_keymap("super+shift+r", "normal", function(_)
	ura.api.reload()
end)

ura.api.set_keymap("super+shift+e", "normal", function(_)
	ura.api.terminate()
end)

ura.api.set_hook("prepare", function(_) end)

ura.api.set_hook("ready", function(_)
	ura.api.spawn("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2")
	ura.api.set_pointer_accel_profile("flat")
end)

ura.api.set_hook("reload", function(_)
	ura.api.spawn("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2")
	ura.api.set_pointer_accel_profile("flat")
end)
