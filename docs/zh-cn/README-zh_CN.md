# Ura

<img src="/assets/icon.png" style="width:30%">

Ura 是一个全新的 wayland 合成器，它基于 wlroots，使用 c++ 编写，并引入 lua（luajit）作为配置系统。

Ura 的优势在于其高度可定制性。通过 hook 机制，它将一部分窗口管理器的功能公开，从而实现在特定流程中注入代码的能力。

Lua 是一门通用语言，它被许多编辑器/LSP（如 lua_ls）支持，因此可以为配置文件添加错误提示、语法高亮、自动补全，从而允许以编写代码的方式来编写合成器的配置文件。

<img src="/assets/show.png" style="width:100%">

## 安装

### 构建

依赖项包括：

- wayland
- wlroots0.19
- luajit
- sol2
- pixman
- cmake
- pkgconf
- nlohmann-json
- [just](https://github.com/casey/just)（可选）
- cargo

```shell
git clone https://github.com/levinion/ura.git
cd ura
just

cd uracil 
cargo build --release
sudo install -Dm755 target/release/uracil /usr/bin/uracil
```

如果你不想使用 just，可以直接使用 cmake 构建：

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

## 配置

Ura的配置文件按照查找顺序排列，有以下几个路径：

- `$XDG_CONFIG_HOME/ura/init.lua`
- `$HOME/.config/ura/init.lua`
- `/etc/ura/init.lua`

[默认的配置文件](/assets/init.lua)会随着ura一同安装到`/etc/ura/init.lua`。若尝试对其进行修改，建议拷贝到用户目录下再进行修改。

默认的终端为 [alacritty](https://github.com/alacritty/alacritty)，这可以使用快捷键`super+t`打开。不过，请确保在启动 ura 之前安装了`alacritty`，或者将其修改为其他你想要使用的终端。

### 快捷键

使用以下方式设置快捷键，快捷键的格式为若干修饰键（可以是 0 个）接一个单键，中间以“+”连接：

```lua
ura.keymap.set("super+t", function()
  os.execute("alacritty -e tmux &")
end)

ura.keymap.set("super+shift+e", function()
  ura.api.terminate()
end)

ura.keymap.set("XF86AudioRaiseVolume", function()
  os.execute("volume -i 5")
end)
```

### 钩子

钩子，或 hook，是 Ura 提供的最强大的机制，它可以在合成器的运行流程中利用公开的 API 执行任意操作，因此能完成许多其他合成器做不到的功能。hook 也为一些高级功能，如窗口规则，提供了一个更优雅的实现。

下面给出一些示例：

`prepare` 钩子运行于 Lua 模块初始化之后，此时尚未创建合成器运行所需的各种资源，因此可以在此设置全局环境变量：

```lua
ura.hook.set("prepare", function()
  ura.fn.set_env("WLR_RENDERER", "gles2")
  ura.fn.set_env("WLR_NO_HARDWARE_CURSORS", "1")
  ura.fn.set_env("LIBVA_DRIVER_NAME", "nvidia")
  ura.fn.set_env("__GLX_VENDOR_LIBRARY_NAME", "nvidia")
end)
```

在 Wayland 建立 Socket，合成器开始运行前，通过 `ready` 钩子（钩子名称可能随版本发生变化，下同）执行 wlr-randr 以设置显示器模式。另外，此处也是创建自启应用的好时机：

```lua
ura.hook.set("ready", function()
  os.execute("wlr-randr --output DP-5 --mode 3840x2160@119.879997Hz --scale 2 &")
end)
```

`focus-change` 和 `workspace-change` 分别发生于焦点和工作区发生变化，此时可以通知桌面组件，如 waybar 自定义模块，进行更新：

```lua
ura.hook.set("focus-change", function()
  os.execute("pkill -SIGRTMIN+9 waybar &")
end)

ura.hook.set("workspace-change", function()
  os.execute("pkill -SIGRTMIN+8 waybar &")
end)
```

`window-new` 发生于新的顶级窗口创建和聚焦后，此时可以设置窗口样式：

```lua
ura.hook.set("window-new", function()
  local win = ura.win.get_current()
  if not win then return end
  if string.match(win.app_id, "fzfmenu") then
    ura.win.set_layout(win.index, "floating")
    ura.win.resize(win.index, 1000, 600)
    ura.win.center(win.index)
  end
end)
```

## 布局

Ura 的 `layout` 模块允许用户自定义布局算法。以下是一个最简单的布局算法，它只是让窗口填满屏幕的可用区域（或称最大化）。

```lua
ura.layout.set("my-tiling", function(index)
  local output = ura.output.get_current()
  return { x = output.usable.x, y = output.usable.y, width = output.usable.width, height = output.usable.height }
end)
```

与 `window-new` hook 配合使用可以在窗口创建时应用用户自定义的布局算法。默认布局算法包括：`tiling`、`floating`、`fullscreen`。其中`tiling`是一个简单的水平平铺算法。

更多示例可以参见：[examples](/examples/)。

如果想要了解更多信息，请访问 [Ura Wiki](https://github.com/levinion/ura/wiki)

## 许可证

本项目许可证采用 [GPLv3](/LICENSE)。
