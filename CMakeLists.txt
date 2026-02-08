# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.19)

project("XMLGameEngine")

add_executable(${PROJECT_NAME})

include(scripts/cmake/options.cmake)
include(scripts/cmake/dependencies.cmake)
include(scripts/cmake/targets.cmake)
include(scripts/cmake/platform.cmake)
include(scripts/cmake/assets.cmake)

target_sources(${PROJECT_NAME} PRIVATE
	"source/main.cpp"
	"source/game.cpp"
	"source/engine.cpp"
	"source/object.cpp"
	"source/states.cpp"
	"source/utils.cpp"
	"source/game_xml.cpp"
	"source/game_expr.cpp"
	"source/game_sfml.cpp"

	"include/game.h"
	"include/engine.h"
	"include/object.h"
	"include/states.h"
	"include/utils.h"
	"include/game_xml.h"
	"include/game_expr.h"
	"include/game_sfml.h"
)