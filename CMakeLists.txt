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

function(target_add_dependence target pkgname)
  pkg_check_modules(${pkgname} REQUIRED ${pkgname})
  target_link_libraries(${target} PRIVATE ${${pkgname}_LIBRARIES})
  target_include_directories(${target} PRIVATE ${${pkgname}_INCLUDE_DIRS})
endfunction()

target_add_dependence(${NAME} wlroots-0.19)
target_add_dependence(${NAME} wayland-server)
target_add_dependence(${NAME} xkbcommon)
target_add_dependence(${NAME} luajit)
target_add_dependence(${NAME} libnotify)
target_add_dependence(${NAME} spdlog)
target_add_dependence(${NAME} libinput)
target_add_dependence(${NAME} nlohmann_json)
target_add_dependence(${NAME} absl_strings)
target_add_dependence(${NAME} absl_inlined_vector)
target_add_dependence(${NAME} absl_flat_hash_map)
target_add_dependence(${NAME} cxxopts)

include(cmake/CPM.cmake)
CPMAddPackage("gh:ThePhD/sol2#c1f95a773c6f8f4fde8ca3efe872e7286afe4444")
target_link_libraries(${NAME} PRIVATE sol2)
