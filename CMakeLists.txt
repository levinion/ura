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

target_add_dependence(${NAME} wlroots-0.20)
target_add_dependence(${NAME} wayland-server)
target_add_dependence(${NAME} xkbcommon)
target_add_dependence(${NAME} luajit)
target_add_dependence(${NAME} libnotify)
target_add_dependence(${NAME} spdlog)
target_add_dependence(${NAME} libinput)
target_add_dependence(${NAME} cxxopts)

include(cmake/CPM.cmake)
CPMAddPackage("gh:ThePhD/sol2#c1f95a773c6f8f4fde8ca3efe872e7286afe4444")
target_link_libraries(${NAME} PRIVATE sol2)

include(CTest)
if(BUILD_TESTING)
  add_executable(ura-json-tests tests/json.cpp src/util/json.cpp)
  target_include_directories(ura-json-tests PRIVATE include/)
  target_include_directories(ura-json-tests SYSTEM PRIVATE ${GLAZE_INCLUDE_DIR})

  add_test(NAME ura-json-primitives COMMAND ura-json-tests primitives)
  add_test(NAME ura-json-integer-ranges COMMAND ura-json-tests integer-ranges)
  add_test(NAME ura-json-nested-round-trip COMMAND ura-json-tests nested-round-trip)
  add_test(NAME ura-json-invalid-input COMMAND ura-json-tests invalid-json)

  pkg_check_modules(WLROOTS_TEST REQUIRED IMPORTED_TARGET wlroots-0.20)
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
