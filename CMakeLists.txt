# project
set(NAME "ura")
set(VERSION 0.3.3)
cmake_minimum_required(VERSION 3.10)
project(${NAME} VERSION ${VERSION} LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)

# src
file(GLOB_RECURSE SRC src/*.c src/*.cpp)
add_executable(${NAME} ${SRC})

if(CMAKE_BUILD_TYPE MATCHES 'Release')
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# include header
target_include_directories(${NAME} PRIVATE include/)
target_include_directories(${NAME} PRIVATE include/protocols/)

# define version
string(TOUPPER "${NAME}" UPPER_NAME)
string(REPLACE "-" "_" UPPER_NAME ${UPPER_NAME})
set(VERSION_MACRO_NAME "${UPPER_NAME}_PROJECT_VERSION")
target_compile_definitions(${NAME} PRIVATE ${VERSION_MACRO_NAME}="${VERSION}")

target_compile_definitions(${NAME} PRIVATE SPDLOG_NO_EXCEPTIONS)
target_compile_definitions(${NAME} PRIVATE CXXOPTS_NO_EXCEPTIONS)
target_compile_definitions(${NAME} PRIVATE WLR_USE_UNSTABLE)
target_compile_options(${NAME} PRIVATE -Wall -Wextra
-Wno-unused-parameter -Wno-array-bounds -fno-exceptions)

find_package(PkgConfig REQUIRED)

find_path(GLAZE_INCLUDE_DIR NAMES glaze/json/generic.hpp)
if(NOT GLAZE_INCLUDE_DIR)
	message(FATAL_ERROR "Glaze headers not found. Install the system Glaze package.")
endif()
target_include_directories(${NAME} SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})

function(target_add_dependence target pkgname)
	pkg_check_modules(${pkgname} REQUIRED ${pkgname})
	target_link_libraries(${target} PRIVATE ${${pkgname}_LIBRARIES})
	target_include_directories(${target} PRIVATE ${${pkgname}_INCLUDE_DIRS})
endfunction()

target_add_dependence(${NAME} egl)
target_add_dependence(${NAME} glesv2)
target_add_dependence(${NAME} wlroots-0.20)
target_add_dependence(${NAME} wayland-server)
target_add_dependence(${NAME} xkbcommon)
target_add_dependence(${NAME} luajit)
target_add_dependence(${NAME} libnotify)
target_add_dependence(${NAME} spdlog)
target_add_dependence(${NAME} libinput)
target_add_dependence(${NAME} cxxopts)
target_add_dependence(${NAME} gbm)
target_add_dependence(${NAME} libdrm)
target_add_dependence(${NAME} pixman-1)

find_path(RLGL_INCLUDE_DIR NAMES rlgl.h)
if(NOT RLGL_INCLUDE_DIR)
	message(FATAL_ERROR "rlgl.h not found. Install raylib (it ships rlgl.h).")
endif()
target_include_directories(${NAME} PRIVATE ${RLGL_INCLUDE_DIR})

include(cmake/CPM.cmake)
CPMAddPackage("gh:ThePhD/sol2#c1f95a773c6f8f4fde8ca3efe872e7286afe4444")
target_link_libraries(${NAME} PRIVATE sol2)

CPMAddPackage(
  NAME EnTT
  GITHUB_REPOSITORY skypjack/entt
  GIT_TAG v3.16.0
)
target_link_libraries(${NAME} PRIVATE EnTT::EnTT)

include(CTest)
if(BUILD_TESTING)
  add_executable(ura-json-tests tests/json.cpp src/util/json.cpp)
  target_include_directories(ura-json-tests PRIVATE include/)
  target_include_directories(ura-json-tests SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})

  add_executable(ura-entity-tests tests/entity.cpp src/core/entity.cpp)
  target_include_directories(ura-entity-tests PRIVATE include/)
  target_include_directories(ura-entity-tests SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})
  target_link_libraries(ura-entity-tests PRIVATE EnTT::EnTT sol2)
  target_add_dependence(ura-entity-tests luajit)

  add_test(NAME ura-core-entity-identity COMMAND ura-entity-tests identity)
  add_test(NAME ura-core-entity-stale-handle COMMAND ura-entity-tests stale-handle)
  add_test(NAME ura-core-entity-userdata COMMAND ura-entity-tests userdata)

  pkg_check_modules(WLROOTS_TEST REQUIRED IMPORTED_TARGET wlroots-0.20)

  add_executable(ura-scene-tests
    tests/scene.cpp
    src/core/entity.cpp
    src/scene/scene.cpp
  )
  target_include_directories(ura-scene-tests PRIVATE
    include/
    ${WLROOTS_TEST_INCLUDE_DIRS}
  )
  target_include_directories(ura-scene-tests SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})
  target_link_libraries(ura-scene-tests PRIVATE EnTT::EnTT sol2)
  target_add_dependence(ura-scene-tests luajit)

  add_test(NAME ura-scene-snapshot-order COMMAND ura-scene-tests snapshot-order)
  add_test(NAME ura-scene-stale-handle COMMAND ura-scene-tests stale-handle)
  add_test(NAME ura-scene-components COMMAND ura-scene-tests components)
  add_test(NAME ura-scene-layer-shell COMMAND ura-scene-tests layer-shell)
  add_test(NAME ura-scene-snapshot-cache COMMAND ura-scene-tests snapshot-cache)
  add_test(NAME ura-scene-session-lock COMMAND ura-scene-tests session-lock)
  add_test(NAME ura-scene-rect COMMAND ura-scene-tests rect)

  add_executable(ura-animation-tests
    tests/animation.cpp
    src/core/entity.cpp
    src/scene/scene.cpp
    src/scene/animation_system.cpp
  )
  target_include_directories(ura-animation-tests PRIVATE
    include/
    ${WLROOTS_TEST_INCLUDE_DIRS}
  )
  target_include_directories(ura-animation-tests SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})
  target_link_libraries(ura-animation-tests PRIVATE EnTT::EnTT sol2)
  target_add_dependence(ura-animation-tests luajit)

  add_test(NAME ura-animation-easing COMMAND ura-animation-tests easing)
  add_test(NAME ura-animation-data-boundary COMMAND ura-animation-tests data-boundary)
  add_test(NAME ura-animation-flip COMMAND ura-animation-tests flip)
  add_test(NAME ura-animation-retarget COMMAND ura-animation-tests retarget)
  add_test(NAME ura-animation-timeout COMMAND ura-animation-tests timeout)
  add_test(NAME ura-animation-opacity COMMAND ura-animation-tests opacity)
  add_test(NAME ura-animation-opacity-retarget COMMAND ura-animation-tests opacity-retarget)
  add_test(NAME ura-animation-transition-move-merge COMMAND ura-animation-tests transition-move-merge)
  add_test(NAME ura-animation-destroy-on-finish COMMAND ura-animation-tests destroy-on-finish)

  find_package(Threads REQUIRED)
  add_executable(ura-worker-pool-tests
    tests/worker_pool.cpp
    src/util/worker_pool.cpp
  )
  target_include_directories(ura-worker-pool-tests PRIVATE include/)
  target_link_libraries(ura-worker-pool-tests PRIVATE Threads::Threads)
  add_test(NAME ura-worker-pool COMMAND ura-worker-pool-tests)

  add_executable(ura-frame-planner-tests
    tests/frame_planner.cpp
    src/scene/frame_planner.cpp
  )
  target_include_directories(ura-frame-planner-tests PRIVATE include/ include/protocols/)
  target_compile_definitions(ura-frame-planner-tests PRIVATE WLR_USE_UNSTABLE)
  target_add_dependence(ura-frame-planner-tests wlroots-0.20)
  target_add_dependence(ura-frame-planner-tests wayland-server)
  add_test(NAME ura-frame-planner COMMAND ura-frame-planner-tests)

  add_test(NAME ura-json-primitives COMMAND ura-json-tests primitives)
  add_test(NAME ura-json-integer-ranges COMMAND ura-json-tests integer-ranges)
  add_test(NAME ura-json-nested-round-trip COMMAND ura-json-tests nested-round-trip)
  add_test(NAME ura-json-invalid-input COMMAND ura-json-tests invalid-json)

  set(RAYLIB_RENDERER_SOURCES
    src/render/raylib/buffer.cpp
    src/render/raylib/effects.cpp
    src/render/raylib/egl.cpp
    src/render/raylib/render_pass.cpp
    src/render/raylib/renderer.cpp
    src/render/raylib/resources.cpp
    src/render/raylib/rlgl_es.cpp
    src/render/raylib/shader.cpp
    src/render/raylib/texture.cpp
  )
  add_executable(ura-raylib-renderer-tests
    tests/raylib_renderer.cpp
    ${RAYLIB_RENDERER_SOURCES}
  )
  target_include_directories(ura-raylib-renderer-tests PRIVATE
    include/
    include/protocols/
    ${RLGL_INCLUDE_DIR}
  )
  target_compile_definitions(ura-raylib-renderer-tests PRIVATE WLR_USE_UNSTABLE)
  target_compile_definitions(ura-raylib-renderer-tests PRIVATE SPDLOG_NO_EXCEPTIONS)
  target_compile_options(ura-raylib-renderer-tests PRIVATE
    -Wall -Wextra -Wno-unused-parameter -fno-exceptions
  )
  target_add_dependence(ura-raylib-renderer-tests wlroots-0.20)
  target_add_dependence(ura-raylib-renderer-tests wayland-server)
  target_add_dependence(ura-raylib-renderer-tests egl)
  target_add_dependence(ura-raylib-renderer-tests glesv2)
  target_add_dependence(ura-raylib-renderer-tests gbm)
  target_add_dependence(ura-raylib-renderer-tests libdrm)
  target_add_dependence(ura-raylib-renderer-tests pixman-1)
  target_add_dependence(ura-raylib-renderer-tests spdlog)
  add_test(NAME ura-raylib-renderer COMMAND ura-raylib-renderer-tests)
  set_tests_properties(ura-raylib-renderer PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(ura-frame-renderer-tests
    tests/frame_renderer.cpp
    src/render/frame_renderer.cpp
    ${RAYLIB_RENDERER_SOURCES}
  )
  target_include_directories(ura-frame-renderer-tests PRIVATE
    include/
    include/protocols/
    ${RLGL_INCLUDE_DIR}
  )
  target_compile_definitions(ura-frame-renderer-tests PRIVATE WLR_USE_UNSTABLE)
  target_compile_definitions(ura-frame-renderer-tests PRIVATE SPDLOG_NO_EXCEPTIONS)
  target_compile_options(ura-frame-renderer-tests PRIVATE
    -Wall -Wextra -Wno-unused-parameter -fno-exceptions
  )
  target_add_dependence(ura-frame-renderer-tests wlroots-0.20)
  target_add_dependence(ura-frame-renderer-tests wayland-server)
  target_add_dependence(ura-frame-renderer-tests egl)
  target_add_dependence(ura-frame-renderer-tests glesv2)
  target_add_dependence(ura-frame-renderer-tests gbm)
  target_add_dependence(ura-frame-renderer-tests libdrm)
  target_add_dependence(ura-frame-renderer-tests pixman-1)
  target_add_dependence(ura-frame-renderer-tests spdlog)
  add_test(NAME ura-frame-renderer COMMAND ura-frame-renderer-tests)
  set_tests_properties(ura-frame-renderer PROPERTIES SKIP_RETURN_CODE 77)

  pkg_check_modules(XKBCOMMON_TEST REQUIRED IMPORTED_TARGET xkbcommon)
  add_executable(ura-util-tests tests/util.cpp)
  target_include_directories(ura-util-tests PRIVATE include/ include/protocols/)
  target_compile_definitions(ura-util-tests PRIVATE WLR_USE_UNSTABLE)
  target_link_libraries(ura-util-tests PRIVATE
    PkgConfig::WLROOTS_TEST
    PkgConfig::XKBCOMMON_TEST
  )
  add_test(NAME ura-util-strings COMMAND ura-util-tests strings)
  add_test(NAME ura-util-rgb COMMAND ura-util-tests rgb)
  add_test(NAME ura-util-keybinding COMMAND ura-util-tests keybinding)
  add_test(NAME ura-core-status COMMAND ura-util-tests status)
  add_test(NAME ura-core-dispatcher-pipe COMMAND ura-util-tests dispatcher-pipe)
  add_test(NAME ura-core-dispatcher-timeout COMMAND ura-util-tests dispatcher-timeout)
  set_tests_properties(ura-core-dispatcher-pipe ura-core-dispatcher-timeout PROPERTIES TIMEOUT 5)

  find_program(LUAJIT_EXECUTABLE luajit)
  if(LUAJIT_EXECUTABLE)
    add_test(
      NAME ura-animation-lua
      COMMAND ${LUAJIT_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/lua/animation.lua
    )
    set_tests_properties(ura-animation-lua PROPERTIES
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endif()

  find_program(BASH_EXECUTABLE bash)
  if(BASH_EXECUTABLE)
    add_test(NAME ura-cmd-syntax COMMAND ${BASH_EXECUTABLE} -n ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/ura-cmd)
    add_test(NAME ura-repl-syntax COMMAND ${BASH_EXECUTABLE} -n ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/ura-repl)
    add_test(NAME ura-cmd-help COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/ura-cmd help)
    add_test(NAME ura-repl-help COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/ura-repl --help)
  endif()

  find_package(Python3 QUIET COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME ura-plugin-help COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/scripts/ura-plugin --help)
  endif()

  find_program(CARGO_EXECUTABLE cargo)
  if(CARGO_EXECUTABLE)
    add_test(NAME ura-shell-cli COMMAND ${CARGO_EXECUTABLE} test --locked --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/tools/ura-shell/Cargo.toml)
    set_tests_properties(ura-shell-cli PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/ura-shell)
  endif()
endif()
