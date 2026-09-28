# CMakeLists.txt 
# XML Game Engine
# author: beefviper
# date: Sept 18, 2020

cmake_minimum_required(VERSION 3.28)

project("XMLGameEngine")

add_executable(${PROJECT_NAME})

include(scripts/cmake/options.cmake)
include(scripts/cmake/dependencies.cmake)
include(scripts/cmake/targets.cmake)
include(scripts/cmake/platform.cmake)
include(scripts/cmake/assets.cmake)

# Everything but main.cpp: shared with the test suite's own executable
# (scripts/cmake/tests.cmake) so the two never drift apart from having the
# file list maintained by hand in two places.
set(ENGINE_SOURCES
	"source/cli.cpp"
	"source/game.cpp"
	"source/engine.cpp"
	"source/object.cpp"
	"source/states.cpp"
	"source/utils.cpp"
	"source/game_xml.cpp"
	"source/game_expr.cpp"
	"source/game_sfml.cpp"
	"source/command.cpp"
	"source/collision_detector.cpp"
	"source/command_executor.cpp"
	"source/window.cpp"
	"source/window_sfml.cpp"
)

set(ENGINE_HEADERS
	"include/cli.h"
	"include/game.h"
	"include/engine.h"
	"include/object.h"
	"include/states.h"
	"include/utils.h"
	"include/game_xml.h"
	"include/game_expr.h"
	"include/game_sfml.h"
	"include/command.h"
	"include/collision_detector.h"
	"include/command_executor.h"
	"include/window.h"
	"include/window_sfml.h"
)

target_sources(${PROJECT_NAME} PRIVATE
	"source/main.cpp"
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

if (BUILD_TESTING)
	include(scripts/cmake/tests.cmake)
endif()
