local M = {}

---@param opt table?
function M.setup(opt)
  local function apply(win)
    if not win:is_mapped() then
      return
    end

    local output = win:output()
    assert(output)
    local usable = output:usable_geometry()
    assert(usable)
    local windows = ura.class.UraWindow:from_tags(output:tags())
    assert(windows)

    local tiling_windows = {}

    for _, w in ipairs(windows) do
      if w:layout() == "tiling" then
        table.insert(tiling_windows, w)
      end
    end

    local sum = #tiling_windows
    if sum == 0 then
      return
    end

    local index = -1
    for i, w in ipairs(tiling_windows) do
      if w == win then
        index = i - 1
        break
      end
    end

    assert(index ~= -1)

    local outer_r = opt and opt.outer_r or 10
    local outer_l = opt and opt.outer_l or 10
    local outer_t = opt and opt.outer_t or 10
    local outer_b = opt and opt.outer_b or 10
    local inner = opt and opt.inner or 10

    local gaps = sum - 1
    local w = (usable.width - (outer_r + outer_l) - inner * gaps) / sum
    local h = usable.height - (outer_t + outer_b)
    local x = usable.x + outer_l + (w + inner) * index
    local y = usable.y + outer_t
    win:resize(w, h)
    win:move(x, y)
  end

  ---@param tags table<string>
  local function apply_all(tags)
    local wins = ura.class.UraWindow:from_tags(tags)
    for _, win in ipairs(wins) do
      if win:layout() == "tiling" then
        apply(win)
      end
    end
  end

  local o = {
    ns = "layout.tiling",
    priority = 100,
  }

  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    local tags = win:output():tags()
    if e.to == "tiling" then
      win:set_z_index(ura.g.layer.normal)
      apply_all(tags)
    elseif e.from == "tiling" then
      apply_all(tags)
    end
  end, o)

  ura.hook.add("window-close", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if win:layout() == "tiling" then
      apply_all(win:output():tags())
    end
  end, o)

  ura.hook.add("window-tags-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if win:layout() == "tiling" then
      apply_all(e.from)
      apply_all(e.to)
    end
  end, o)

  ura.hook.add("output-tags-change", function(e)
    apply_all(e.to)
  end, o)

  ura.hook.add("output-usable-geometry-change", function(e)
    local output = ura.class.UraOutput:new(e.id)
    apply_all(output:tags())
  end, o)
end

return M
