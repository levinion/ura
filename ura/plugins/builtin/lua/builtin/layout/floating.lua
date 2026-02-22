local M = {}

function M.setup()
  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "floating" then
      win:set_z_index(ura.g.layer.floating)
      -- recover window size
      local userdata = win:userdata()
      assert(userdata)
      assert(ura.fn.validate(userdata, "floating", "table"))
      if userdata.floating.width and userdata.floating.height then
        win:resize(userdata.floating.width, userdata.floating.height)
      end
      if userdata.floating.x and userdata.floating.y then
        win:move(userdata.floating.x, userdata.floating.y)
      end
    elseif e.from == "floating" then
      local geo = win:geometry()
      assert(geo)
      win:update_userdata({ floating = geo })
    end
  end, {
    ns = "layout.floating",
    priority = ura.g.priority.instant,
  })

  ura.hook.add("window-new", function(e)
    local win = ura.class.UraWindow:new(e.id)
    assert(win)
    local geo = win:geometry()
    assert(geo)
    win:update_userdata({ floating = geo })
  end, { ns = "layout.floating" })

  -- move window
  pcall(function()
    local anchor = nil
    local timer = nil
    local w = nil

    local function move_window()
      local pos = ura.api.get_cursor_pos()
      assert(w)
      local geo = w:geometry()
      assert(geo)
      assert(anchor)
      w:move(geo.x + (pos.x - anchor.x), geo.y + (pos.y - anchor.y))
      anchor = pos
    end

    ura.hook.add("mouse-key", function(e)
      if e.id ~= ura.api.get_keybinding_id("super+mouseleft") then
        return
      end
      if e.state == "pressed" then
        w = ura.class.UraWindow:current()
        assert(w)
        if w:layout() ~= "floating" then
          return
        end
        anchor = ura.api.get_cursor_pos()
        timer = ura.fn.set_interval(move_window, 10)
      else
        if timer then
          ura.fn.clear_interval(timer)
          timer = nil
          move_window()
        end
      end
    end)
  end)

  -- resize window
  pcall(function()
    local anchor = nil
    local w = nil
    local timer = nil

    local function resize_window()
      local pos = ura.api.get_cursor_pos()
      assert(w)
      local geo = w:geometry()
      assert(geo)
      assert(anchor)
      w:resize(geo.width + (pos.x - anchor.x), geo.height + (pos.y - anchor.y))
      anchor = pos
    end

    ura.hook.add("mouse-key", function(e)
      if ura.fn.lshift(e.id, 32) ~= ura.fn.lshift(ura.api.get_keybinding_id("super+mouseright"), 32) then
        return
      end
      if e.state == "pressed" then
        w = ura.class.UraWindow:current()
        assert(w)
        if w:layout() ~= "floating" then
          return
        end
        anchor = ura.api.get_cursor_pos()
        timer = ura.fn.set_interval(resize_window, 10)
      else
        if timer then
          ura.fn.clear_interval(timer)
          timer = nil
          resize_window()
        end
      end
    end)
  end)
end

return M
