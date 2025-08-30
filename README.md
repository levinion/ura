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
- cmake
- pkgconf
- nlohmann-json
- cli11
- [just](https://github.com/casey/just) (optional)
- cargo

```shell
git clone https://github.com/levinion/ura.git
cd ura
just

cd uracil 
cargo build --release
sudo install -Dm755 target/release/uracil /usr/bin/uracil
```

If you prefer not to use `just`, you can build with CMake directly:

```shell
git clone https://github.com/levinion/ura.git
cd ura
mkdir -p include/protocols
wayland-scanner server-header ./protocols/xdg-shell.xml include/protocols/xdg-shell-protocol.h
wayland-scanner server-header ./protocols/wlr-layer-shell-unstable-v1.xml include/protocols/wlr-layer-shell-unstable-v1-protocol.h
wayland-scanner server-header ./protocols/wlr-output-power-management-unstable-v1.xml include/protocols/wlr-output-power-management-unstable-v1-protocol.h
wayland-scanner server-header ./protocols/cursor-shape-v1.xml include/protocols/cursor-shape-v1-protocol.h
wayland-scanner server-header ./protocols/pointer-constraints-unstable-v1.xml include/protocols/pointer-constraints-unstable-v1-protocol.h
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

sudo install -Dm755 "build/ura" "/usr/bin/ura"
sudo install -Dm644 "LICENSE" "/usr/share/licenses/ura/LICENSE"
sudo install -d "/etc/ura"
sudo install -Dm644 "assets/init.lua" "/etc/ura/"
sudo install -Dm644 "assets/ura.desktop" "/usr/share/wayland-sessions/ura.desktop"
sudo install -d "/usr/share/lua/5.1"
sudo cp -r "lua/ura" "/usr/share/lua/5.1/"

cd uracil 
cargo build --release
sudo install -Dm755 target/release/uracil /usr/bin/uracil
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
  os.execute("foot -e tmux &")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("XF86AudioRaiseVolume", function()
  os.execute("volume -i 5")
end)
```

### Hooks

Hooks are Ura’s most powerful feature. They allow arbitrary operations to be performed using public APIs during the compositor’s runtime, enabling features that other compositors typically can’t support. Hooks also offer a cleaner way to implement advanced features such as window rules.

Here are some examples:

The `prepare` hook runs after the Lua module is initialized, but before compositor resources are created. You can use this hook to set global environment variables:

```lua
ura.hook.set("prepare", function()
  ura.fn.set_env("WLR_RENDERER", "vulkan")
  ura.fn.set_env("WLR_NO_HARDWARE_CURSORS", "0")
  ura.fn.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.fn.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)
```

Use the `ready` hook (note: hook names may change across versions) to start applications just before the compositor starts running:

```lua
ura.hook.set("ready", function()
  ura.fn.set_env("DISPLAY", ":0")
  os.execute("xwayland-satellite &")
end)
```

The `focus-change` and `workspace-change` hooks are triggered when focus or workspace changes. You can notify desktop components (e.g., custom modules in waybar) to update:

```lua
ura.hook.set("focus-change", function()
  os.execute("pkill -SIGRTMIN+9 waybar &")
end)

ura.hook.set("workspace-change", function()
  os.execute("pkill -SIGRTMIN+8 waybar &")
end)
```

The `window-new` hook is triggered when a new top-level window is created and focused. You can use it to apply window-specific styling:

```lua
ura.hook.set("window-new", function(index)
  local win = ura.win.get(index)
  if not win then return end
  if string.match(win.app_id, "fzfmenu") then
    ura.win.set_layout(win.index, "floating")
    ura.win.resize(win.index, 1000, 600)
    ura.win.center(win.index)
  end
end)
```

## Layout

The layout module in Ura lets you create custom layout algorithms. Here's a simple example that makes a window fill the entire usable area of the screen (just like maximizing).

```lua
ura.layout.set("my-tiling", function(index)
  local output = ura.output.get_current()
  return { x = output.usable.x, y = output.usable.y, width = output.usable.width, height = output.usable.height }
end)
```
Used with the window-new hook, this allows you to apply a custom layout algorithm when a new window is created. The default layout algorithms include tiling, floating, and fullscreen. Of these, tiling is a simple horizontal tiling algorithm.

More examples are available at: [examples](/examples/)

For more infomation, please visit [Ura Wiki](https://github.com/levinion/ura/wiki)

## License

This project is licensed under the terms of the [GPLv3](/LICENSE).
