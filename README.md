# Ura

<img src="/assets/icon.png" style="width:30%">

[中文文档](/docs/zh-cn/README-zh_CN.md)

**Ura** is a brand-new Wayland compositor built on **wlroots**, written in **C++**, and uses **Lua (LuaJIT)** as its configuration system.

The strength of Ura lies in its high customizability. Through a **hook mechanism**, it exposes part of the window manager’s functionality, allowing you to inject code into specific workflows.

Lua is a general-purpose language supported by many editors and LSPs (like `lua_ls`). This enables features such as error checking, syntax highlighting, and auto-completion for configuration files, allowing you to configure the compositor as if you're writing actual code.

<img src="/assets/show.png" style="width:100%">

## Installation

### Build

Dependencies include:

- wayland
- wlroots0.19
- luajit
- sol2
- pixman
- nlohmann-json
- cli11
- libnotify
- spdlog

Make dependencies include:

- cargo
- make
- cmake
- pkgconf

```shell
git clone https://github.com/levinion/ura.git
cd ura
make

cd shell 
cargo build --release
sudo install -Dm755 target/release/ura-shell /usr/bin/ura-shell
```

### AUR

```shell
paru/yay -S ura-git
```

## Configuration

Ura's configuration files are searched in the following order:

- `$XDG_CONFIG_HOME/ura/init.lua`
- `$HOME/.config/ura/init.lua`
- `/etc/ura/init.lua`

The [default configuration file](/assets/init.lua) is installed with Ura at `/etc/ura/init.lua`. If you wish to modify it, it's recommended to copy it to your user directory before making changes.

The default terminal is [foot](https://codeberg.org/dnkl/foot), which can be launched using the `super+t` shortcut. However, make sure that `foot` is installed before starting Ura, or change it to another terminal of your choice.

### Keybindings

Define keybindings using the following format: one or more modifier keys (or none) followed by a single key, connected by `"+"`:

```lua
ura.keymap.set("super+t", function()
  ura.api.spawn("foot -e tmux")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("XF86AudioRaiseVolume", function()
  ura.api.spawn("volume -i 5")
end)
```

### Hooks

Hooks are Ura’s most powerful feature. They allow arbitrary operations to be performed using public APIs during the compositor’s runtime, enabling features that other compositors typically can’t support. Hooks also offer a cleaner way to implement advanced features such as window rules.

Here are some examples:

The `prepare` hook runs after the Lua module is initialized, but before compositor resources are created. You can use this hook to set global environment variables:

```lua
ura.hook.set("prepare", function(_)
  ura.api.set_env("WLR_RENDERER", "vulkan")
  ura.api.set_env("WLR_NO_HARDWARE_CURSORS", "0")
  ura.api.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.api.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)
```

Use the `ready` hook (note: hook names may change across versions) to start applications just before the compositor starts running:

```lua
ura.hook.set("ready", function(_)
  ura.api.set_env("DISPLAY", ":0")
  ura.api.spawn("xwayland-satellite")
end)
```

The `focus-change` and `workspace-change` hooks are triggered when focus or workspace changes. You can notify desktop components (e.g., custom modules in waybar) to update:

```lua
ura.hook.set("focus-change", function(_)
  ura.api.spawn("pkill -SIGRTMIN+9 waybar")
end)

ura.hook.set("workspace-change", function(_)
  ura.api.spawn("pkill -SIGRTMIN+8 waybar")
end)
```

The `window-new` hook is triggered when a new top-level window is created and focused. You can use it to apply window-specific styling:

```lua
ura.hook.set("window-new", function(e)
	local app_id = ura.api.get_window_app_id(e.id)
	assert(app_id)
	if string.match(app_id, "fzfmenu") then
		ura.layout.set(e.id, "floating")
		ura.api.resize_window(e.id, 1000, 600)
		ura.win.center(e.id)
	end
end, {})
```

More hooks could be found in the WIKI (TODO)。

## Layout

The layout module in Ura lets you create custom layout algorithms. This is how ura implement the fullscreen layout.

```lua
ura.layout.register("fullscreen", {
  enter = function(win)
    ura.api.set_window_draggable(win, false)
    ura.api.set_window_z_index(win, 250)
    ura.api.set_window_fullscreen(win, true)
  end,
  apply = function(win)
    local output = ura.api.get_window_output(win)
    assert(output)
    local geo = ura.api.get_output_logical_geometry(output)
    assert(geo)
    ura.api.resize_window(win, geo.width, geo.height)
    ura.api.move_window(win, geo.x, geo.y)
  end,
  leave = function(win)
    ura.api.set_window_fullscreen(win, false)
  end,
})
end)
```

Used with the window-new hook, this allows you to apply a custom layout algorithm when a new window is created. The default layout algorithms include tiling, floating, and fullscreen. Of these, tiling is a simple horizontal tiling algorithm. The buildin layout algorithms are at [lua/ura/_runtime/layout.lua](/lua/ura/_runtime/layout.lua)

More configuration examples are available at: [examples](/examples/)

For more infomation, please visit [Ura Wiki](https://github.com/levinion/ura/wiki)

## License

This project is licensed under the terms of the [GPLv3](/LICENSE).
