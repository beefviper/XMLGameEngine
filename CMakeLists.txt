# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set (CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE})

project ("XMLGameEngine")

# Include sub-projects.
add_subdirectory ("XMLGameEngine")
