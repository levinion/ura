local workspaces = ura.api.get_workspaces()
assert(workspaces)
for _, ws in ipairs(workspaces) do
	local wins = ura.api.get_windows(ws)
	if wins then
		for k, win in ipairs(wins) do
			if win then
				local app_id = ura.api.get_window_app_id(win)
				local title = ura.api.get_window_title(win)
				if not ura.api.is_workspace_named(ws) then
					local workspace_index = ura.api.get_workspace_index(ws)
					if workspace_index then
						print(app_id, title, workspace_index, k)
					end
				else
					local workspace_name = ura.api.get_workspace_name(ws)
					if workspace_name then
						print(app_id, title, workspace_name, k)
					end
				end
			end
		end
	end
end
