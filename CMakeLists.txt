# project
set(name "ura")
cmake_minimum_required(VERSION 3.10)
project(${name} CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)

# src
file(GLOB_RECURSE SRC src/*.cpp)
add_executable(${name} ${SRC})

# link std library staticly
target_compile_options(${name} PRIVATE)
target_link_options(${name} PRIVATE -static-libgcc -static-libstdc++)

if(CMAKE_BUILD_TYPE MATCHES 'Release')
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# include header
target_include_directories(${name} PUBLIC include/)
target_include_directories(${name} PUBLIC include/protocols/)

find_package(PkgConfig REQUIRED)

target_compile_definitions(${name} PUBLIC WLR_USE_UNSTABLE)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.19)
target_link_libraries(${name} PUBLIC ${WLROOTS_LIBRARIES})
target_include_directories(${name} PUBLIC ${WLROOTS_INCLUDE_DIRS})

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
target_link_libraries(${name} PUBLIC ${WAYLAND_SERVER_LIBRARIES})
target_include_directories(${name} PUBLIC ${WAYLAND_SERVER_INCLUDE_DIRS})

pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
target_link_libraries(${name} PUBLIC ${XKBCOMMON_LIBRARIES})
target_include_directories(${name} PUBLIC ${XKBCOMMON_INCLUDE_DIRS})

pkg_check_modules(SOL2 REQUIRED sol2)
target_link_libraries(${name} PUBLIC ${SOL2_LIBRARIES})
target_include_directories(${name} PUBLIC ${SOL2_INCLUDE_DIRS})

pkg_check_modules(LUAJIT REQUIRED luajit)
target_link_libraries(${name} PUBLIC ${LUAJIT_LIBRARIES})
target_include_directories(${name} PUBLIC ${LUAJIT_INCLUDE_DIRS})

pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
target_link_libraries(${name} PUBLIC ${NLOHMANN_JSON_LIBRARIES})
target_include_directories(${name} PUBLIC ${NLOHMANN_JSON_INCLUDE_DIRS})

pkg_check_modules(CLI11 REQUIRED CLI11)
target_link_libraries(${name} PUBLIC ${CLI11_LIBRARIES})
target_link_directories(${name} PUBLIC ${CLI11_INCLUDE_DIRS})
