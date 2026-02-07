# project
set(NAME "ura")
set(VERSION 0.3.0)
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

target_compile_definitions(${NAME} PRIVATE WLR_USE_UNSTABLE)

target_compile_options(${NAME} PRIVATE -Wall)

find_package(PkgConfig REQUIRED)

function(target_add_dependence target pkgname)
  pkg_check_modules(${pkgname} REQUIRED ${pkgname})
  target_link_libraries(${target} PRIVATE ${${pkgname}_LIBRARIES})
  target_include_directories(${target} PRIVATE ${${pkgname}_INCLUDE_DIRS})
endfunction()

target_add_dependence(${NAME} wlroots-0.19)
target_add_dependence(${NAME} wayland-server)
target_add_dependence(${NAME} xkbcommon)
target_add_dependence(${NAME} sol2)
target_add_dependence(${NAME} luajit)
target_add_dependence(${NAME} CLI11)
target_add_dependence(${NAME} libnotify)
target_add_dependence(${NAME} spdlog)
target_add_dependence(${NAME} libinput)
target_add_dependence(${NAME} nlohmann_json)
