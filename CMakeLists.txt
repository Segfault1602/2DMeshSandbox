cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

project(mesh2d_testbench)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)

if (APPLE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

include(./fetch_dependency.cmake)

SET(CMAKE_CXX_FLAGS_RELEASE  "-O3 -ffast-math -flto -DNDEBUG -std=c++20 -march=native -fsave-optimization-record")
# SET(CMAKE_CXX_FLAGS_DEBUG  "-Og -g -DDEBUG -std=c++20 -march=native -fsanitize=address -fno-omit-frame-pointer")


set(BUILD_SHARED OFF CACHE BOOL "")
set(COMPILE_PROJECTS OFF CACHE BOOL "")
set(ENABLE_JACK OFF CACHE BOOL "")
set(INSTALL_HEADERS OFF CACHE BOOL "")
add_subdirectory(stk)
set(STK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stk/include)

add_subdirectory(src)