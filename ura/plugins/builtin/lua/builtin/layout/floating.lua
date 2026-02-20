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
    priority = 0,
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

    local function interval_move()
      move_window()
      timer = ura.api.set_timeout(interval_move, 10)
    end

    ura.keymap.set({ "super+mouseleft" }, function()
      w = ura.class.UraWindow:current()
      assert(w)
      if w:layout() ~= "floating" then
        return
      end
      anchor = ura.api.get_cursor_pos()
      timer = ura.api.set_timeout(interval_move, 10)
    end)

    ura.keymap.set({
      "super+mouseleft",
    }, function()
      if timer then
        ura.api.clear_timeout(timer)
        move_window()
      end
    end, { state = "released" })
  end)

  -- resize window
  pcall(function()
    local anchor = nil
    local timer = nil
    local w = nil

    local function resize_window()
      local pos = ura.api.get_cursor_pos()
      assert(w)
      local geo = w:geometry()
      assert(geo)
      assert(anchor)
      w:resize(geo.width + (pos.x - anchor.x), geo.height + (pos.y - anchor.y))
      anchor = pos
    end

    local function interval_resize()
      resize_window()
      timer = ura.api.set_timeout(interval_resize, 10)
    end

    ura.keymap.set({ "super+mouseright" }, function()
      w = ura.class.UraWindow:current()
      assert(w)
      if w:layout() ~= "floating" then
        return
      end
      anchor = ura.api.get_cursor_pos()
      timer = ura.api.set_timeout(interval_resize, 10)
    end)

    ura.keymap.set({ "super+mouseright", "mouseright" }, function()
      if timer then
        ura.api.clear_timeout(timer)
        resize_window()
        return true
      end
    end, { state = "released" })
  end)
end

return M
