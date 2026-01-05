local M = {}

local function master_stack(opt)
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
      local m_fact = opt and opt.master_factor or 0.6

      local tiling_wins = {}
      local win_index = -1
      for i = 0, #windows - 1 do
        local w = ura.api.get_window(workspace, i)
        if w and ura.layout.get(w) == "tiling" then
          table.insert(tiling_wins, w)
          if w == win then
            win_index = #tiling_wins
          end
        end
      end

      local n = #tiling_wins
      if n == 0 or win_index == -1 then
        return
      end

      local full_w = usable.width - outer_l - outer_r
      local full_h = usable.height - outer_t - outer_b
      local start_x = usable.x + outer_l
      local start_y = usable.y + outer_t

      local x, y, w, h

      if n == 1 then
        x, y, w, h = start_x, start_y, full_w, full_h
      elseif win_index == 1 then
        w = (full_w - inner) * m_fact
        h = full_h
        x = start_x
        y = start_y
      else
        local stack_n = n - 1
        w = (full_w - inner) * (1 - m_fact)
        h = (full_h - (stack_n - 1) * inner) / stack_n
        x = start_x + (full_w - w)
        y = start_y + (win_index - 2) * (h + inner)
      end

      ura.api.resize_window(win, w, h)
      ura.api.move_window(win, x, y)
    end,
  })

  if not opt or opt.keymap ~= false then
    -- focus master
    ura.keymap.set("super+h", function()
      local ws = ura.api.get_current_workspace()
      assert(ws)
      local win = ura.api.get_window(ws, 0)
      assert(win)
      ura.api.focus_window(win)
    end)
    -- focus stack
    ura.keymap.set("super+l", function()
      local current = ura.api.get_current_window()
      assert(current)
      if ura.api.get_window_index(current) == 0 then
        local ws = ura.api.get_current_workspace()
        assert(ws)
        local win = ura.api.get_window(ws, 1)
        assert(win)
        ura.api.focus_window(win)
      end
    end)
    -- down stack
    ura.keymap.set("super+j", function()
      local current = ura.api.get_current_window()
      assert(current)
      if ura.api.get_window_index(current) > 0 then
        ura.cmd.focus_next()
      end
    end)
    -- up stack
    ura.keymap.set("super+k", function()
      local current = ura.api.get_current_window()
      assert(current)
      if ura.api.get_window_index(current) > 1 then
        ura.cmd.focus_prev()
      end
    end)
    -- move to master
    ura.keymap.set("super+shift+h", function()
      local current = ura.api.get_current_window()
      assert(current)
      if ura.api.get_window_index(current) ~= 0 then
        local ws = ura.api.get_current_workspace()
        assert(ws)
        local main = ura.api.get_window(ws, 0)
        assert(main)
        ura.api.swap_window(current, main)
      end
    end)
    -- move to stack
    ura.keymap.set("super+shift+l", function()
      local current = ura.api.get_current_window()
      assert(current)
      if ura.api.get_window_index(current) == 0 then
        ura.cmd.swap_next()
      end
    end)
  end
end

local function spiral(opt)
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

      local tiling_windows = {}
      local win_index = -1
      for i = 0, #windows - 1 do
        local w = ura.api.get_window(workspace, i)
        if w and ura.layout.get(w) == "tiling" then
          table.insert(tiling_windows, w)
          if w == win then
            win_index = #tiling_windows
          end
        end
      end

      local total = #tiling_windows
      if total == 0 then
        return
      end

      local curr_x = usable.x + outer_l
      local curr_y = usable.y + outer_t
      local curr_w = usable.width - outer_l - outer_r
      local curr_h = usable.height - outer_t - outer_b

      for i = 1, total do
        local is_last = (i == total)
        local split_horizontal = (i % 2 == 1)

        local draw_w, draw_h = curr_w, curr_h

        if not is_last then
          if split_horizontal then
            draw_w = (curr_w - inner) / 2
          else
            draw_h = (curr_h - inner) / 2
          end
        end

        if i == win_index then
          ura.api.resize_window(win, draw_w, draw_h)
          ura.api.move_window(win, curr_x, curr_y)
          break
        end

        if split_horizontal then
          curr_x = curr_x + draw_w + inner
          curr_w = curr_w - draw_w - inner
        else
          curr_y = curr_y + draw_h + inner
          curr_h = curr_h - draw_h - inner
        end
      end
    end,
  })

  if not opt or opt.keymap ~= false then
    ura.keymap.set("super+h", function()
      ura.cmd.focus_prev()
    end)
    ura.keymap.set("super+l", function()
      ura.cmd.focus_next()
    end)
    ura.keymap.set("super+shift+h", function()
      ura.cmd.swap_prev()
    end)
    ura.keymap.set("super+shift+l", function()
      ura.cmd.swap_next()
    end)
  end
end

local function vertical(opt)
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

  if not opt or opt.keymap ~= false then
    ura.keymap.set("super+h", function()
      ura.cmd.focus_prev()
    end)
    ura.keymap.set("super+l", function()
      ura.cmd.focus_next()
    end)
    ura.keymap.set("super+shift+h", function()
      ura.cmd.swap_prev()
    end)
    ura.keymap.set("super+shift+l", function()
      ura.cmd.swap_next()
    end)
  end
end

---@param opt table?
function M.setup(opt)
  if opt and opt.type == "master&stack" then
    master_stack(opt)
  elseif opt and opt.type == "spiral" then
    spiral(opt)
  else
    vertical(opt)
  end

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
