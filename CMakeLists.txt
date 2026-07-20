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

	"source/core/engine.cpp"
	"source/core/game.cpp"
	
	"source/core/engine/system_clock.cpp"
	"source/core/engine/system_console.cpp"
	"source/core/engine/system_file.cpp"
	"source/core/engine/system_memory.cpp"
	"source/core/engine/system_thread.cpp"

	"source/core/engine/service_timer.cpp"
	"source/core/engine/service_logger.cpp"
	"source/core/engine/service_resource.cpp"

	"source/core/engine/loop_input.cpp"
	"source/core/engine/loop_render.cpp"
	"source/core/engine/loop_update.cpp"
	
	"source/core/game/compile_parse.cpp"
	"source/core/game/compile_validate.cpp"
	"source/core/game/compile_evaluate.cpp"
	"source/core/game/compile_generate.cpp"
	
	"source/core/game/runtime_object.cpp"
	"source/core/game/runtime_states.cpp"
	
	"source/window/window_factory.cpp"
	"source/window/window_sfml3.cpp"
	"source/window/window_sdl2.cpp"
	
	"source/xml/xml_factory.cpp"
	"source/xml/xml_xerces.cpp"
	"source/xml/xml_tinyxml2.cpp"
	
	"include/core/types.h"
	"include/core/engine.h"
	"include/core/game.h"
	
	"include/core/engine/system_clock.h"
	"include/core/engine/system_console.h"
	"include/core/engine/system_file.h"
	"include/core/engine/system_memory.h"
	"include/core/engine/system_thread.h"

	"include/core/engine/service_timer.h"
	"include/core/engine/service_logger.h"
	"include/core/engine/service_resource.h"

	"include/core/engine/loop_input.h"
	"include/core/engine/loop_render.h"
	"include/core/engine/loop_update.h"
	
	"include/core/game/compile_parse.h"
	"include/core/game/compile_validate.h"
	"include/core/game/compile_evaluate.h"
	"include/core/game/compile_generate.h"

	"include/core/game/runtime_object.h"
	"include/core/game/runtime_states.h"
	
	"include/window/window_interface.h"
	"include/window/window_factory.h"
	"include/window/window_sfml3.h"
	"include/window/window_sdl2.h"

	"include/xml/xml_interface.h"
	"include/xml/xml_factory.h"
	"include/xml/xml_xerces.h"
	"include/xml/xml_tinyxml2.h"
)