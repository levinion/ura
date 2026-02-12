local M = {}

local function apply(win, opt)
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

local function apply_all(opt)
  local wins = ura.class.UraWindow:from_tags(ura.class.UraOutput:current():tags())
  for _, win in ipairs(wins) do
    if win:layout() == "tiling" then
      apply(win, opt)
    end
  end
end

---@param opt table?
function M.setup(opt)
  ura.hook.add("window-layout-change", function(e)
    local win = ura.class.UraWindow:new(e.id)
    if e.to == "tiling" then
      win:set_draggable(false)
      win:set_z_index(100)
      apply_all(opt)
    elseif e.from == "tiling" then
      apply_all(opt)
    end
  end, { ns = "layout.tiling" })

  for _, name in ipairs({
    "window-close",
    "window-tags-change",
  }) do
    ura.hook.add(name, function(e)
      if ura.class.UraWindow:new(e.id):layout() == "tiling" then
        apply_all(opt)
      end
    end, { ns = "layout.tiling" })
  end

  ura.hook.add("output-usable-geometry-change", function(_)
    apply_all(opt)
  end, { ns = "layout.tiling" })
end

return M
