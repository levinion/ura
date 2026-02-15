ura.opt = require("ura.opt")
ura.fn = require("ura.fn")
ura.hook = require("ura.hook")
ura.keymap = require("ura.keymap")
ura.class = require("ura.class")
ura.cmd = require("ura.cmd")

ura.fn.load_dir("/usr/share/ura/plugins")
ura.fn.load_dir("~/.local/share/ura/plugins")
ura.fn.load("~/.config/ura")
ura.fn.load("$XDG_CONFIG_HOME/ura")

local function find_config_path()
  local xdg_config = os.getenv("XDG_CONFIG_HOME")
  local home = os.getenv("HOME")
  local root = nil

  if xdg_config and xdg_config ~= "" then
    root = xdg_config
  elseif home then
    root = home .. "/.config"
  end

  if root ~= nil then
    local dotfile = root .. "/ura/init.lua"
    if ura.fn.exists(dotfile) then
      return dotfile
    end
  end

  local global_dotfile = "/etc/ura/init.lua"
  if ura.fn.exists(global_dotfile) then
    return global_dotfile
  end

  return nil
end

local function load_config()
  local path = find_config_path()

  if not path then
    return nil, "could not find any config files, exiting..."
  end

  local config_env = {}
  setmetatable(config_env, { __index = _G })

  local chunk, err = loadfile(path)
  if not chunk then
    return nil, "Syntax Error: " .. (err or "unknown")
  end

  setfenv(chunk, config_env)
  local success, run_err = pcall(chunk)

  if not success then
    return nil, "Runtime Error: " .. (run_err or "unknown")
  end

  return true
end

local status, err = load_config()
if not status then
  error(err)
end
