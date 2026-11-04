project(
  'ura',
  ['c', 'cpp'],
  version: '0.3.3',
  meson_version: '>=1.8.0',
  default_options: [
    'cpp_std=c++26',
    'warning_level=2',
    'buildtype=release',
    'prefix=/usr',
    'sysconfdir=/etc',
  ],
)

cc = meson.get_compiler('cpp')
fs = import('fs')

entt_dep = dependency('entt', required: false)
if not entt_dep.found()
  if not cc.has_header('entt/entt.hpp')
    error('EnTT headers not found')
  endif
  entt_dep = declare_dependency()
endif

glaze_dep = declare_dependency()
if not cc.has_header('glaze/glaze.hpp')
  error('Glaze headers not found')
endif

add_project_arguments(
  '-DURA_PROJECT_VERSION="' + meson.project_version() + '"',
  '-DWLR_USE_UNSTABLE',
  '-DSPDLOG_NO_EXCEPTIONS',
  '-DCXXOPTS_NO_EXCEPTIONS',
  language: ['c', 'cpp'],
)
add_project_arguments(
  '-fno-exceptions',
  '-Wno-unused-parameter',
  '-Wno-array-bounds',
  '-Wno-keyword-macro',
  language: 'cpp',
)


deps = [
  dependency('egl'),
  dependency('glesv2'),
  dependency('wlroots-0.20'),
  dependency('wayland-server'),
  dependency('xkbcommon'),
  dependency('luajit'),
  dependency('libnotify'),
  dependency('spdlog'),
  dependency('libinput'),
  dependency('cxxopts'),
  dependency('gbm'),
  dependency('libdrm'),
  dependency('pixman-1'),
  dependency('raylib'),
  dependency('threads'),
  entt_dep,
  glaze_dep,
  dependency('sol2', fallback: ['sol2', 'sol2_dep']),
]

scanner = find_program('wayland-scanner')

protocol_xml = run_command(
  'find',
  'protocols',
  '-maxdepth',
  '1',
  '-type',
  'f',
  '-name',
  '*.xml',
  check: true,
).stdout().strip().split('\n')

protocol_headers = []
foreach xml : protocol_xml
  name = fs.stem(fs.name(xml))
  protocol_headers += custom_target(
    name + '-server-header',
    input: xml,
    output: name + '-protocol.h',
    command: [scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach

ipc_protocol = custom_target(
  'ura-ipc-code',
  input: 'protocols/ura-ipc.xml',
  output: 'ipc.c',
  command: [scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

source_paths = run_command(
  'find',
  'src',
  '-type',
  'f',
  '-name',
  '*.cpp',
  check: true,
).stdout().strip().split('\n')

sources = files(source_paths)

raylib_paths = run_command(
  'find',
  'src/render/raylib',
  '-type',
  'f',
  '-name',
  '*.cpp',
  check: true,
).stdout().strip().split('\n')

raylib_sources = files(raylib_paths)

include_dirs = include_directories('include', '.')

ura = executable(
  'ura',
  sources,
  ipc_protocol,
  protocol_headers,
  dependencies: deps,
  include_directories: include_dirs,
  install: true,
)


install_subdir('ura', install_dir: get_option('datadir'))
install_data(
  'assets/init.lua',
  install_dir: get_option('sysconfdir') / 'ura',
)
install_data(
  'assets/ura.desktop',
  install_dir: get_option('datadir') / 'wayland-sessions',
)
install_data(
  'assets/completions/zsh/_ura',
  'assets/completions/zsh/_ura-shell',
  'assets/completions/zsh/_ura-plugin',
  'assets/completions/zsh/_ura-cmd',
  install_dir: get_option('datadir') / 'zsh/site-functions',
)
install_data(
  'tools/scripts/ura-cmd',
  'tools/scripts/ura-plugin',
  'tools/scripts/ura-repl',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

cargo = find_program('cargo', required: false)
if cargo.found()
  manifest = meson.project_source_root() / 'tools/ura-shell/Cargo.toml'
  ura_shell = custom_target(
    'ura-shell',
    output: 'ura-shell',
    command: [
      find_program('sh'),
      '-c',
      'cargo build --release --locked --manifest-path "$1" '
        + '&& cp "$(dirname "$1")/target/release/ura-shell" "$2"',
      'sh',
      manifest,
      '@OUTPUT@',
    ],
    build_by_default: true,
    depend_files: files(
      'tools/ura-shell/Cargo.toml',
      'tools/ura-shell/Cargo.lock',
      'tools/ura-shell/src/main.rs',
    ),
    install: true,
    install_dir: get_option('bindir'),
  )
endif

subdir('tests')
