# project
set(NAME "ura")
set(VERSION 0.1.4)
cmake_minimum_required(VERSION 3.10)
project(${NAME} VERSION ${VERSION} LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)

# src
file(GLOB_RECURSE SRC src/*.cpp)
add_executable(${NAME} ${SRC})

if(CMAKE_BUILD_TYPE MATCHES 'Release')
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# include header
target_include_directories(${NAME} PRIVATE include/)
target_include_directories(${NAME} PRIVATE include/protocols/)

string(TOUPPER "${NAME}" UPPER_NAME)
string(REPLACE "-" "_" UPPER_NAME ${UPPER_NAME})
set(VERSION_MACRO_NAME "${UPPER_NAME}_PROJECT_VERSION")
target_compile_definitions(${NAME} PRIVATE ${VERSION_MACRO_NAME}="${VERSION}")

find_package(PkgConfig REQUIRED)

target_compile_definitions(${NAME} PRIVATE WLR_USE_UNSTABLE)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.19)
target_link_libraries(${NAME} PRIVATE ${WLROOTS_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${WLROOTS_INCLUDE_DIRS})

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
target_link_libraries(${NAME} PRIVATE ${WAYLAND_SERVER_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${WAYLAND_SERVER_INCLUDE_DIRS})

pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
target_link_libraries(${NAME} PRIVATE ${XKBCOMMON_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${XKBCOMMON_INCLUDE_DIRS})

pkg_check_modules(SOL2 REQUIRED sol2)
target_link_libraries(${NAME} PRIVATE ${SOL2_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${SOL2_INCLUDE_DIRS})

pkg_check_modules(LUAJIT REQUIRED luajit)
target_link_libraries(${NAME} PRIVATE ${LUAJIT_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${LUAJIT_INCLUDE_DIRS})

pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
target_link_libraries(${NAME} PRIVATE ${NLOHMANN_JSON_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})

pkg_check_modules(CLI11 REQUIRED CLI11)
target_link_libraries(${NAME} PRIVATE ${CLI11_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${CLI11_INCLUDE_DIRS})

pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
target_link_libraries(${NAME} PRIVATE ${LIBNOTIFY_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${LIBNOTIFY_INCLUDE_DIRS})

pkg_check_modules(SPDLOG REQUIRED spdlog)
target_link_libraries(${NAME} PRIVATE ${SPDLOG_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${SPDLOG_INCLUDE_DIRS})

pkg_check_modules(LIBINPUT REQUIRED libinput)
target_link_libraries(${NAME} PRIVATE ${LIBINPUT_LIBRARIES})
target_include_directories(${NAME} PRIVATE ${LIBINPUT_INCLUDE_DIRS})
