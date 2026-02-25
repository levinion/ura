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
      win:update_userdata(function(t)
        t.floating = ura.fn.copy(win:geometry())
      end)
    end
  end, {
    ns = "layout.floating",
    priority = ura.g.priority.instant,
  })

  ura.hook.add("window-new", function(e)
    local win = ura.class.UraWindow:new(e.id)
    assert(win)
    win:update_userdata(function(t)
      t.floating = ura.fn.copy(win:geometry())
    end)
  end, { ns = "layout.floating", priority = ura.g.priority.instant })

  local function listen_mouse_key(key, f)
    local anchor = nil
    local timer = nil
    local w = nil

    local function func(win)
      local pos = ura.api.get_cursor_pos()
      assert(anchor)
      f(win, pos.x - anchor.x, pos.y - anchor.y)
      anchor = pos
    end

    ura.hook.add("mouse-key", function(e)
      if e.state == "pressed" then
        if e.id ~= ura.api.get_keybinding_id(key) then
          return
        end
        w = ura.class.UraWindow:current()
        assert(w)
        if w:layout() ~= "floating" then
          return
        end
        anchor = ura.api.get_cursor_pos()
        timer = ura.api.set_interval(function()
          func(w)
        end, 10)
        return false
      else
        if ura.fn.lshift(e.id, 32) ~= ura.fn.lshift(ura.api.get_keybinding_id(key), 32) then
          return
        end
        if timer then
          ura.api.clear_interval(timer)
          timer = nil
          func(w)
          return false
        end
      end
    end, { ns = "layout.floating" })
  end

  listen_mouse_key("super+mouseleft", function(w, x, y)
    local geo = w:geometry()
    w:move(geo.x + x, geo.y + y)
  end)

  listen_mouse_key("super+mouseright", function(w, width, height)
    local geo = w:geometry()
    w:resize(geo.width + width, geo.height + height)
  end)
end

return M
