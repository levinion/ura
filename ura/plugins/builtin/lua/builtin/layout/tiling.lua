local M = {}

---@param opt table?
function M.setup(opt)
  ura.layout.register("tiling", {
    enter = function(win)
      ura.api.set_window_draggable(win, false)
      ura.api.set_window_z_index(win, 100)
    end,
    apply = function(win)
      local output = ura.api.get_window_output(win)
      assert(output)
      local usable = ura.api.get_output_usable_geometry(output)
      assert(usable)
      local workspace = ura.api.get_window_workspace(win)
      assert(workspace)
      local windows = ura.api.get_windows(workspace)
      assert(windows)

      local outer_r = opt and opt.outer_r or 10
      local outer_l = opt and opt.outer_l or 10
      local outer_t = opt and opt.outer_t or 10
      local outer_b = opt and opt.outer_b or 10
      local inner = opt and opt.inner or 10

      local sum = 0
      local index = 0

      for i = 0, #windows - 1 do
        local w = ura.api.get_window(workspace, i)
        if w then
          if w == win then
            index = sum
          end
          local layout = ura.layout.get(w)
          if layout == "tiling" then
            sum = sum + 1
          end
        end
      end

      local gaps = sum - 1
      local w = (usable.width - (outer_r + outer_l) - inner * gaps) / sum
      local h = usable.height - (outer_t + outer_b)
      local x = usable.x + outer_l + (w + inner) * index
      local y = usable.y + outer_t
      ura.api.resize_window(win, w, h)
      ura.api.move_window(win, x, y)
    end,
  })

  ura.hook.set("window-new", function(e)
    local win = e.id
    assert(win)
    if ura.layout.get(win) ~= nil then
      return
    end
    ura.layout.set(win, "tiling")
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 100, desc = "set tiling as the fallback layout" })

  ura.hook.set("window-resize", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window resized" })

  ura.hook.set("window-close", function(_)
    -- we don't know this window's layout since it is destroyed
    -- we have to apply all of the windows
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window closed" })

  ura.hook.set("workspace-change", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as workspace changed" })

  ura.hook.set("window-swap", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window's index is changed" })

  ura.hook.set("output-usable-geometry-change", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as usable geometry change" })

  ura.hook.set("window-remove", function(_)
    local ws = ura.api.get_current_workspace()
    assert(ws)
    ura.layout.apply_workspace(ws)
  end, { ns = "layout.tiling", priority = 40, desc = "re-apply layout as window removed" })

  ura.hook.set("window-new", function(e)
    ura.api.focus_window(e.id)
  end, { ns = "focus", priority = 0, desc = "focus when window created" })
end

return M
