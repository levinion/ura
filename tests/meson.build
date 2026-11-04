test_includes = include_directories('../include', '..')
test_deps = deps

json_tests = executable(
  'ura-json-tests',
  'json.cpp',
  '../src/util/json.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-json-primitives', json_tests, args: ['primitives'])
test('ura-json-integer-ranges', json_tests, args: ['integer-ranges'])
test('ura-json-nested-round-trip', json_tests, args: ['nested-round-trip'])
test('ura-json-invalid-input', json_tests, args: ['invalid-json'])

entity_tests = executable(
  'ura-entity-tests',
  'entity.cpp',
  '../src/core/entity.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-core-entity-identity', entity_tests, args: ['identity'])
test('ura-core-entity-stale-handle', entity_tests, args: ['stale-handle'])
test('ura-core-entity-userdata', entity_tests, args: ['userdata'])

effect_registry_tests = executable(
  'ura-effect-registry-tests',
  'effect_registry.cpp',
  '../src/render/effect_registry.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-effect-registry', effect_registry_tests)

scene_tests = executable(
  'ura-scene-tests',
  'scene.cpp',
  '../src/core/entity.cpp',
  '../src/scene/scene.cpp',
  '../src/util/tag.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-scene-snapshot-order', scene_tests, args: ['snapshot-order'])
test('ura-scene-stale-handle', scene_tests, args: ['stale-handle'])
test('ura-scene-components', scene_tests, args: ['components'])
test('ura-scene-layer-shell', scene_tests, args: ['layer-shell'])
test('ura-scene-snapshot-cache', scene_tests, args: ['snapshot-cache'])
test('ura-scene-session-lock', scene_tests, args: ['session-lock'])
test('ura-scene-rect', scene_tests, args: ['rect'])
test('ura-scene-rotation', scene_tests, args: ['rotation'])

animation_tests = executable(
  'ura-animation-tests',
  'animation.cpp',
  '../src/core/entity.cpp',
  '../src/scene/scene.cpp',
  '../src/scene/animation_system.cpp',
  '../src/util/tag.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-animation-easing', animation_tests, args: ['easing'])
test('ura-animation-data-boundary', animation_tests, args: ['data-boundary'])
test('ura-animation-flip', animation_tests, args: ['flip'])
test('ura-animation-retarget', animation_tests, args: ['retarget'])
test('ura-animation-timeout', animation_tests, args: ['timeout'])
test('ura-animation-opacity', animation_tests, args: ['opacity'])
test('ura-animation-opacity-retarget', animation_tests, args: ['opacity-retarget'])
test('ura-animation-transition-move-merge', animation_tests, args: ['transition-move-merge'])
test('ura-animation-destroy-on-finish', animation_tests, args: ['destroy-on-finish'])
test('ura-animation-effect-uniform', animation_tests, args: ['effect-uniform'])
test('ura-animation-rotation', animation_tests, args: ['rotation'])

worker_pool_tests = executable(
  'ura-worker-pool-tests',
  'worker_pool.cpp',
  '../src/util/worker_pool.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-worker-pool', worker_pool_tests)

frame_planner_tests = executable(
  'ura-frame-planner-tests',
  'frame_planner.cpp',
  '../src/scene/frame_planner.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-frame-planner', frame_planner_tests)

util_tests = executable(
  'ura-util-tests',
  'util.cpp',
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-util-strings', util_tests, args: ['strings'])
test('ura-util-rgb', util_tests, args: ['rgb'])
test('ura-util-keybinding', util_tests, args: ['keybinding'])
test('ura-core-status', util_tests, args: ['status'])
test('ura-core-dispatcher-pipe', util_tests, args: ['dispatcher-pipe'], timeout: 5)
test('ura-core-dispatcher-timeout', util_tests, args: ['dispatcher-timeout'], timeout: 5)

raylib_renderer_tests = executable(
  'ura-raylib-renderer-tests',
  'raylib_renderer.cpp',
  raylib_sources,
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-raylib-renderer', raylib_renderer_tests, timeout: 60)

frame_renderer_tests = executable(
  'ura-frame-renderer-tests',
  'frame_renderer.cpp',
  '../src/render/frame_renderer.cpp',
  raylib_sources,
  dependencies: test_deps,
  include_directories: test_includes,
)
test('ura-frame-renderer', frame_renderer_tests, timeout: 60)

luajit = find_program('luajit', required: false)
if luajit.found()
  test(
    'ura-animation-lua',
    luajit,
    args: [files('lua/animation.lua')],
    workdir: meson.project_source_root(),
  )
endif

bash = find_program('bash', required: false)
if bash.found()
  test('ura-cmd-syntax', bash, args: ['-n', files('../tools/scripts/ura-cmd')])
  test('ura-repl-syntax', bash, args: ['-n', files('../tools/scripts/ura-repl')])
  test('ura-cmd-help', bash, args: [files('../tools/scripts/ura-cmd'), 'help'])
  test('ura-repl-help', bash, args: [files('../tools/scripts/ura-repl'), '--help'])
endif

python = find_program('python3', required: false)
if python.found()
  test(
    'ura-plugin-help',
    python,
    args: [files('../tools/scripts/ura-plugin'), '--help'],
  )
endif

cargo = find_program('cargo', required: false)
if cargo.found()
  test(
    'ura-shell-cli',
    cargo,
    args: [
      'test',
      '--locked',
      '--manifest-path',
      files('../tools/ura-shell/Cargo.toml'),
    ],
    workdir: meson.current_source_dir() / '../tools/ura-shell',
  )
endif
