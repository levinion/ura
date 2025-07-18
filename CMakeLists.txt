# project
set(name "ura")
cmake_minimum_required(VERSION 3.10)
project(${name} CXX)

# clang setting
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_CXX_STANDARD 26)

# src
file(GLOB_RECURSE SRC src/*.cpp)
add_executable(${name} ${SRC})

# link std library staticly
target_compile_options(${name} PRIVATE)
target_link_options(${name} PRIVATE -static-libgcc -static-libstdc++)

# include header
target_include_directories(${name} PUBLIC include/)
target_include_directories(${name} PUBLIC include/protocols/)

# add third party dependencies here
# add_subdirectory(./vendor/hierro2d-cpp)
# target_link_libraries(${name} PUBLIC hierro)

find_package(PkgConfig REQUIRED)

target_compile_definitions(${name} PUBLIC WLR_USE_UNSTABLE)
pkg_check_modules(wlroots REQUIRED wlroots-0.19)
target_link_libraries(${name} PUBLIC ${wlroots_LIBRARIES})
target_include_directories(${name} PUBLIC ${wlroots_INCLUDE_DIRS})

pkg_check_modules(wayland-server REQUIRED wayland-server)
target_link_libraries(${name} PUBLIC ${wayland-server_LIBRARIES})

pkg_check_modules(xkbcommon REQUIRED xkbcommon)
target_link_libraries(${name} PUBLIC ${xkbcommon_LIBRARIES})

pkg_check_modules(sol2 REQUIRED sol2)
target_link_libraries(${name} PUBLIC ${sol2_LIBRARIES})
target_include_directories(${name} PUBLIC ${sol2_INCLUDE_DIRS})

pkg_check_modules(luajit REQUIRED luajit)
target_link_libraries(${name} PUBLIC ${luajit_LIBRARIES})
target_include_directories(${name} PUBLIC ${luajit_INCLUDE_DIRS})
