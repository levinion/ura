local utils = require "utils"

local rules = {
  { app_id = "fzfmenu", layout = "floating", width = 1000, height = 600, center = true },
  { app_id = "scrcpy",  layout = "floating", width = 640,  height = 360, center = true },
  { app_id = "zenity",  layout = "floating", center = true },
  { app_id = "mpv",     layout = "floating", width = 1280, height = 720, center = true },
  { app_id = "XEyes",   layout = "floating", center = true },
}

utils.window_rules(rules)
