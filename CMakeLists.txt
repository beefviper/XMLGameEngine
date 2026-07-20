# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.19)

project("XMLGameEngine")

add_executable(${PROJECT_NAME})

include(scripts/cmake/options.cmake)
include(scripts/cmake/dependencies.cmake)
include(scripts/cmake/sources.cmake)
include(scripts/cmake/platform.cmake)
include(scripts/cmake/assets.cmake)
include(scripts/cmake/targets.cmake)
