# project
set(NAME "ura")
cmake_minimum_required(VERSION 3.10)
project(${NAME} CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)

# src
file(GLOB_RECURSE SRC src/*.cpp)
add_executable(${NAME} ${SRC})

# link std library staticly
target_compile_options(${NAME} PRIVATE)
target_link_options(${NAME} PRIVATE -static-libgcc -static-libstdc++)

if(CMAKE_BUILD_TYPE MATCHES 'Release')
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# include header
target_include_directories(${NAME} PUBLIC include/)
target_include_directories(${NAME} PUBLIC include/protocols/)

find_package(PkgConfig REQUIRED)

target_compile_definitions(${NAME} PUBLIC WLR_USE_UNSTABLE)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.19)
target_link_libraries(${NAME} PUBLIC ${WLROOTS_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${WLROOTS_INCLUDE_DIRS})

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
target_link_libraries(${NAME} PUBLIC ${WAYLAND_SERVER_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${WAYLAND_SERVER_INCLUDE_DIRS})

pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
target_link_libraries(${NAME} PUBLIC ${XKBCOMMON_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${XKBCOMMON_INCLUDE_DIRS})

pkg_check_modules(SOL2 REQUIRED sol2)
target_link_libraries(${NAME} PUBLIC ${SOL2_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${SOL2_INCLUDE_DIRS})

pkg_check_modules(LUAJIT REQUIRED luajit)
target_link_libraries(${NAME} PUBLIC ${LUAJIT_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${LUAJIT_INCLUDE_DIRS})

pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
target_link_libraries(${NAME} PUBLIC ${NLOHMANN_JSON_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${NLOHMANN_JSON_INCLUDE_DIRS})

pkg_check_modules(CLI11 REQUIRED CLI11)
target_link_libraries(${NAME} PUBLIC ${CLI11_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${CLI11_INCLUDE_DIRS})

pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
target_link_libraries(${NAME} PUBLIC ${LIBNOTIFY_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${LIBNOTIFY_INCLUDE_DIRS})

pkg_check_modules(SPDLOG REQUIRED spdlog)
target_link_libraries(${NAME} PUBLIC ${SPDLOG_LIBRARIES})
target_include_directories(${NAME} PUBLIC ${SPDLOG_INCLUDE_DIRS})
